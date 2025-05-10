#include "CdbgExpr.h"
#include <sstream>
#include <bit>
#include <cstdint>
#include <iostream>
#include <vector>
#include "SymbolDescriptor.h"
#include <regex>

namespace CdbgExpr
{
    DbgData *SymbolDescriptor::data = nullptr;
    bool SymbolDescriptor::assignmentAllowed = false;

    std::vector<CType> CType::parseCTypeVector(const std::string& typeStr, bool& isUnsigned)
    {
        std::vector<CType> result;
        std::istringstream iss(typeStr);
        std::string word;
        bool lastWordLong = false;

        while (iss >> word)
        {
            if (word == "*")
            {
                result.insert(result.begin(), CType(CType::Type::POINTER));
            }
            else if (word == "int")
            {
                result.push_back(CType(CType::Type::INT));
            }
            else if (word == "float")
            {
                result.push_back(CType(CType::Type::FLOAT));
            }
            else if (word == "double")
            {
                result.push_back(CType(CType::Type::DOUBLE));
            }
            else if (word == "char")
            {
                result.push_back(CType(CType::Type::CHAR));
            }
            else if (word == "bool")
            {
                result.push_back(CType(CType::Type::BOOL));
            }
            else if (word == "void")
            {
                result.push_back(CType(CType::Type::VOID));
            }
            else if (word == "long")
            {
                if (lastWordLong)
                {
                    result.push_back(CType(CType::Type::LONGLONG));
                    lastWordLong = false;
                }
                else
                {
                    lastWordLong = true;
                }
                continue;
            }
            else if (word == "unsigned")
            {
                isUnsigned = true;
            }
            else if (word == "signed")
            {
                isUnsigned = false;
            }
            else
            {
                // Assume user-defined struct/union type
                result.insert(result.begin(), CType(CType::Type::STRUCT, word));
            }
            if (lastWordLong)
            {
                result.push_back(CType(CType::Type::LONG));
                lastWordLong = false;
            }
        }

        return result;
    }

    SymbolDescriptor::SymbolDescriptor(const char* s)
    {
        fromString(std::string(s));
    }

    SymbolDescriptor::SymbolDescriptor(const std::string &s)
    {
        fromString(s);
    }

    SymbolDescriptor::SymbolDescriptor(double d)
    {
        fromDouble(d);
    }

    SymbolDescriptor::SymbolDescriptor(int64_t i)
    {
        fromInt(i);
    }

    SymbolDescriptor::SymbolDescriptor(uint64_t u)
    {
        fromUint(u);
    }

    std::variant<uint64_t, int64_t, double, float> SymbolDescriptor::getRealValue(const std::vector<uint64_t> &offset) const
    {
        auto val = getConstLiteral(offset);
        std::variant<uint64_t, int64_t, double, float> result;
        switch (val.cType.back().type)
        {
        case CType::Type::DOUBLE:
            result = value_to_double_b(val.getValue());
            break;
        case CType::Type::FLOAT:
            result = value_to_double_b(val.getValue());
            break;
        default:
            result = isSigned ? (int64_t)val.getValue() : val.getValue();
            break;
        }
        return result;
    }

    uint64_t SymbolDescriptor::value_to_uint64_n(uint64_t val, CType type)
    {
        switch (type.type)
        {
        case CType::Type::DOUBLE:
            return static_cast<uint64_t>(std::bit_cast<double>(val));
        case CType::Type::FLOAT:
            return static_cast<uint64_t>(std::bit_cast<float>((uint32_t)val));
        default:
            break;
        }
        return val;
    }
    int64_t SymbolDescriptor::value_to_int64_n(uint64_t val, CType type)
    {
        switch (type.type)
        {
        case CType::Type::DOUBLE:
            return static_cast<int64_t>(std::bit_cast<double>(val));
        case CType::Type::FLOAT:
            return static_cast<int64_t>(std::bit_cast<float>((uint32_t)val));
        default:
            break;
        }
        return val;
    }
    double SymbolDescriptor::value_to_double_b(uint64_t val)
    {
        return std::bit_cast<double>(val);
    }
    double SymbolDescriptor::value_to_double_n(uint64_t val, CType type, bool isSigned)
    {
        switch (type.type)
        {
        case CType::Type::DOUBLE:
            return std::bit_cast<double>(val);
        case CType::Type::FLOAT:
            return std::bit_cast<float>((uint32_t)val);
        default:
            break;
        }
        if (isSigned)
        {
            return (int64_t)val;
        }
        return val;
    }
    float SymbolDescriptor::value_to_float_b(uint64_t val)
    {
        return std::bit_cast<float>((uint32_t)val);
    }
    float SymbolDescriptor::value_to_float_n(uint64_t val, CType type, bool isSigned)
    {
        switch (type.type)
        {
        case CType::Type::DOUBLE:
            return std::bit_cast<double>(val);
        case CType::Type::FLOAT:
            return std::bit_cast<float>((uint32_t)val);
        default:
            break;
        }
        if (isSigned)
        {
            return (int64_t)val;
        }
        return val;
    }

    size_t SymbolDescriptor::getItemSize(const std::vector<CType> &cType, uint8_t level)
    {
        if (cType.size() <= level)
        {
            return 0;
        }
        if (cType[level] == CType::Type::ARRAY)
        {
            return cType[level].size * getItemSize(cType, level + 1);
        }
        return data->CTypeSize(cType[level]);
    }

    CType SymbolDescriptor::promoteType(const CType &left, const CType &right)
    {
        if (data == nullptr)
        {
            throw std::runtime_error("DbgData pointer is null");
        }
        if (left == CType::Type::DOUBLE || left == CType::Type::FLOAT ||
            right == CType::Type::DOUBLE || right == CType::Type::FLOAT) 
        {
            return CType::Type::DOUBLE;
        }
        if (left == CType::Type::POINTER || right == CType::Type::POINTER)
        {
            return CType::Type::POINTER;
        }
        return (data->CTypeSize(left) > data->CTypeSize(right)) ? left : right;
    }
    
    void SymbolDescriptor::fromString(const std::string &str)
    {
        std::string s = str;
        cType.clear();
        hasAddress = false;
        isSigned = false;
    
        if (s.empty())
        {
            cType.push_back(CType::Type::INT);
            value = (int64_t)0;
            return;
        }
    
        // Handle boolean literals
        if (s == "true")
        {
            cType.push_back(CType::Type::BOOL);
            value = (int64_t)1;
            return;
        }
        else if (s == "false")
        {
            cType.push_back(CType::Type::BOOL);
            value = (int64_t)0;
            return;
        }
    
        // Handle integer suffixes (u, l, ul, ull, etc)
        std::regex intRegex(R"(^[-+]?0[xX][0-9a-fA-F]+([uU]?[lL]{0,2}|[lL]{1,2}[uU]?)?$|^[-+]?0[bB][01]+([uU]?[lL]{0,2}|[lL]{1,2}[uU]?)?$|^[-+]?[0-9]+([uU]?[lL]{0,2}|[lL]{1,2}[uU]?)?$)");
        if (std::regex_match(s, intRegex))
        {
            // Strip suffixes
            std::string digits = s;
            std::string suffix;
    
            // Find where suffix starts
            size_t suffixStart = s.find_last_of("0123456789xXbBabcdefABCDEF");
            if (suffixStart != std::string::npos && suffixStart + 1 < s.size())
            {
                suffix = s.substr(suffixStart + 1);
                digits = s.substr(0, suffixStart + 1);
            }
    
            // Determine base
            int base = 10;
            if (digits.find("0x") == 0 || digits.find("0X") == 0) base = 16;
            else if (digits.find("0b") == 0 || digits.find("0B") == 0) {
                base = 2;
                // strtoull doesn't support binary, so manual parsing
                uint64_t bin = 0;
                size_t i = (digits[0] == '-') ? 3 : 2; // skip "0b" or "-0b"
                for (; i < digits.size(); ++i) {
                    if (digits[i] != '0' && digits[i] != '1')
                        throw std::runtime_error("Invalid binary literal: " + s);
                    bin = (bin << 1) | (digits[i] - '0');
                }
                if (digits[0] == '-') {
                    isSigned = true;
                    bin = uint64_t(-(int64_t(bin)));
                }
                cType.push_back(CType::Type::LONGLONG); // customize if needed
                value = bin;
                return;
            } else if (digits[0] == '0' && digits.size() > 1 && digits[1] != 'x' && digits[1] != 'X') base = 8;
    
            // Parse the number
            char* endptr = nullptr;
            uint64_t parsed = std::strtoull(digits.c_str(), &endptr, base);
    
            // Determine signedness and size
            std::string lowered;
            for (char ch : suffix)
                lowered += std::tolower(ch);
    
            if (lowered.find('u') != std::string::npos)
                isSigned = false;
            else
                isSigned = true;
    
            if (lowered.find("ll") != std::string::npos)
                cType.push_back(CType::Type::LONGLONG);
            else if (lowered.find('l') != std::string::npos)
                cType.push_back(CType::Type::LONG);
            else
                cType.push_back(CType::Type::INT);
    
            if (isSigned)
                value = int64_t(parsed);
            else
                value = parsed;
    
            return;
        }
    
        // Handle float literals with suffixes (f, l, etc)
        std::regex floatRegex(R"(^[-+]?(\d+(\.\d*)?|\.\d+)([eE][-+]?\d+)?[fFlL]?$)");
        if (std::regex_match(s, floatRegex))
        {
            char* endptr;
            double d = std::strtod(s.c_str(), &endptr);
            if (endptr && (*endptr == 'f' || *endptr == 'F'))
            {
                cType.push_back(CType::Type::FLOAT);
                value = static_cast<float>(d);
            }
            else
            {
                cType.push_back(CType::Type::DOUBLE);
                value = d;
            }
            return;
        }
    
        // Fallback: treat as INT 0
        cType.push_back(CType::Type::INT);
        value = int64_t(0);
    }    

    void SymbolDescriptor::fromDouble(const double &val)
    {
        hasAddress = false;
        cType.push_back(CType::Type::DOUBLE);
        value = val;
    }

    void SymbolDescriptor::fromInt(const int64_t &val)
    {
        hasAddress = false;
        cType.push_back(CType::Type::LONGLONG);
        value = val;
        isSigned = true;
    }

    void SymbolDescriptor::fromUint(const uint64_t &val)
    {
        hasAddress = false;
        cType.push_back(CType::Type::LONGLONG);
        value = val;
    }

    SymbolDescriptor SymbolDescriptor::dereference(int offset) const
    {
        if (!data)
            throw std::runtime_error("DbgData pointer is null");

        if (cType.empty())
            throw std::runtime_error("Type stack is empty");

        SymbolDescriptor result;
        result.hasAddress = true;
        result.cType = cType;
        result.isSigned = isSigned;

        CType top = cType[0];

        if (top == CType::Type::POINTER)
        {
            if (cType.size() < 2)
                throw std::runtime_error("Pointer type has no pointee");

            // Read the actual pointer value from memory
            uint64_t pointedAddr = getValue();

            result.cType.erase(result.cType.begin()); // Remove POINTER layer
            result.hasAddress = false;
            result.stack = false;
            result.regs.clear();
            
            size_t size = getItemSize(result.cType);
            result.size = size;

            result.setAddr(pointedAddr + offset * size);
        }
        else if (top == CType::Type::ARRAY)
        {
            if (cType.size() < 2)
                throw std::runtime_error("Array type has no element type");

            uint64_t pointedAddr = getValue();

            result.cType.erase(result.cType.begin()); // Remove ARRAY layer
            result.hasAddress = false;
            result.stack = false;
            result.regs.clear();
            
            size_t size = getItemSize(result.cType);
            result.size = size;

            if (cType[1] == CType::Type::ARRAY)
            {
                result.setValue(pointedAddr + offset * size);
            }
            else
            {
                result.setAddr(pointedAddr + offset * size);
            }

        }
        else
        {
            throw std::runtime_error("Cannot dereference a non-pointer, non-array type");
        }

        return result;
    }

    SymbolDescriptor SymbolDescriptor::getMember(const std::string &name) const
    {
        auto it = members.find(name);
        if (it == members.end())
        {
            throw std::runtime_error("Member not found");
        }
        return (it->second.symbol);
    }

    SymbolDescriptor SymbolDescriptor::addressOf() const
    {
        if (data == nullptr)
        {
            throw std::runtime_error("DbgData pointer is null");
        }
        uint64_t addr;
        if (!hasAddress)
        {
            addr = data->invalidAddress;
        }
        else
        {
            addr = value;
        }
        SymbolDescriptor result;
        result.cType = cType;
        result.cType.insert(result.cType.begin(), CType::Type::POINTER);
        result.value = addr;
        result.hasAddress = false;
        result.size = getItemSize(result.cType);
        return result;
    }

    void SymbolDescriptor::setAddr(uint64_t addr)
    {
        value = addr;
        hasAddress = true;
        stack = false;
        regs.clear();
    }

    void SymbolDescriptor::setValue(uint64_t val)
    {
        if (data == nullptr)
        {
            throw std::runtime_error("DbgData pointer is null");
        }
        if (regs.size())
        {
            for (uint64_t i = 0; i < regs.size() && i < 8; i++)
            {
                data->setRegContent(regs[i], ((val >> (i * 8)) & 0xFF));
            }
        }
        if (hasAddress || stack)
        {
            if (cType.empty())
            {
                return;
            }
            uint64_t addr = value;
            if (stack)
            {
                addr = data->getStackPointer() + stackOffs;
            }
            setValueAt(addr, val);
        }
        else
        {
            value = val;
        }
    }
    uint64_t SymbolDescriptor::getValue() const
    {
        if (data == nullptr)
        {
            throw std::runtime_error("DbgData pointer is null");
        }
        uint64_t val = 0;
        if (regs.size())
        {
            for (uint64_t i = 0; i < regs.size() && i < 8; i++)
            {
                val |= (uint64_t)data->getRegContent(regs[i]) << (i * 8);
            }
        }
        else if (hasAddress || stack)
        {
            if (cType.empty())
            {
                return val;
            }
            uint64_t addr = value;
            if (stack)
            {
                addr = data->getStackPointer() + stackOffs;
            }
            val = getValueAt(addr);
        }
        else
        {
            return value;
        }
        return val;
    }
    void SymbolDescriptor::setValueAt(uint64_t addr, uint64_t val, uint8_t level)
    {
        if (level >= cType.size()) throw std::out_of_range("Invalid cType level");
        for (uint64_t i = 0; i < data->CTypeSize(cType[level]); i++)
        {
            data->setByte(addr + i, ((val >> (i * 8)) & 0xFF));
        }
    }
    uint64_t SymbolDescriptor::getValueAt(uint64_t addr, uint8_t level) const
    {
        if (level >= cType.size()) throw std::out_of_range("Invalid cType level");

        uint64_t val = 0;
        for (uint64_t i = 0; i < data->CTypeSize(cType[level]); i++)
        {
            val |= (uint64_t)data->getByte(addr + i) << (i * 8);
        }
        return val;
    }

    std::string SymbolDescriptor::typeOf() const
    {
        if (data == nullptr)
        {
            throw std::runtime_error("DbgData pointer is null");
        }
        if (cType.size() <= 0)
            return "<unknown type>";
        std::ostringstream result;

        result << "(";
        uint64_t i = 0;
        while (i < cType.size() && (cType[i] == CType::Type::POINTER || cType[i] == CType::Type::ARRAY))
        {
            i++;
        }
        if (i >= cType.size())
        {
            result << "<unknown type>";
            return result.str();
        }
        if (!isSigned && cType[i] != CType::Type::STRUCT && cType[i] != CType::Type::BOOL &&
            cType[i] != CType::Type::FLOAT && cType[i] != CType::Type::DOUBLE && cType[i] != CType::Type::VOID)
        {
            result << "unsigned ";
        }
        size_t start = i;
        for (; i < cType.size(); i++)
        {
            if (cType[i] == CType::Type::POINTER)
            {
                result << "*";
                continue;
            }

            if (i != start)
            {
                result << " ";
            }
            
            if (cType[i] == CType::Type::VOID)
            {
                result << "void";
            }
            else if (cType[i] == CType::Type::STRUCT)
            {
                result << "struct";
            }
            else if (cType[i] == CType::Type::CHAR)
            {
                result << "char";
            }
            else if (cType[i] == CType::Type::BOOL)
            {
                result << "bool";
            }
            else if (cType[i] == CType::Type::SHORT)
            {
                result << "short";
            }
            else if (cType[i] == CType::Type::INT)
            {
                result << "int";
            }
            else if (cType[i] == CType::Type::LONG)
            {
                result << "long";
            }
            else if (cType[i] == CType::Type::LONGLONG)
            {
                result << "long long";
            }
            else if (cType[i] == CType::Type::FLOAT)
            {
                result << "float";
            }
            else if (cType[i] == CType::Type::DOUBLE)
            {
                result << "double";
            }
        }

        i = 0;
        while (i < cType.size() && (cType[i] == CType::Type::POINTER || cType[i] == CType::Type::ARRAY))
        {
            if (cType[i] == CType::Type::ARRAY)
            {
                result << "[" << cType[i].size << "]";
            }
            else
            {
                result << "*";
            }
            i++;
        }
        result << ")";
        return result.str();
    }

    std::string SymbolDescriptor::toString() const
    {
        if (data == nullptr)
        {
            throw std::runtime_error("DbgData pointer is null");
        }
        if (cType.size() < 1)
            return "<unknown type>";

        std::ostringstream result;

        if (cType[0] == CType::Type::POINTER)
        {
            if (cType.size() < 2)
                return "*<unknown type>";

            if (cType[1] == CType::Type::CHAR)
            {
                if (!getValue())
                {
                    return "0x0";
                }
                else
                {
                    result << "0x" << std::hex << getValue();
                }
                result << " \"";
                uint64_t addr = getValue();
                char ch;
                while ((ch = static_cast<char>(data->getByte(addr++))) != '\0')
                {
                    result << ch;
                }
                result << "\"";
                return result.str();
            }
            else
            {
                result << typeOf();
                result << "0x" << std::hex << getValue();
                return result.str();
            }
        }
        else if (cType[0] == CType::Type::ARRAY)
        {
            if (cType.size() < 2)
                return "<unknown type>[]";
            result << "[";
            for (size_t i = 0; i < cType[0].size; i++)
            {
                result << dereference(i).toString();
                if (i != cType[0].size - 1)
                {
                    result << ", ";
                }
            }
            result << "]";
            return result.str();
        }
        else if (cType[0] == CType::Type::STRUCT)
        {
            result << cType[0].name;
            result << "{";
            for (auto& member : members)
            {
                result << member.first << " = ";
                result << member.second.symbol.toString();
                result << ", ";
            }
            result << "}";
            return result.str();
        }
        else
        {
            return result.str() + std::visit([](auto && value) 
                { return std::to_string(value); }, getRealValue());
        }

        return "<unknown type>";
    }

    SymbolDescriptor SymbolDescriptor::assign(const SymbolDescriptor &right)
    {
        if (!assignmentAllowed)
        {
            throw std::runtime_error("Assignment not allowed"); 
        }
        setValue(right.getValue());
        return *this;
    }

    SymbolDescriptor SymbolDescriptor::getConstLiteral(const std::vector<uint64_t>& offset) const
    {
        SymbolDescriptor result = *this;
        for (size_t i = 0; i < offset.size() && result.cType.size() && 
            (result.cType[0] == CType::Type::ARRAY || result.cType[0] == CType::Type::POINTER); i++)
        {
            result = result.dereference(offset[i]);
        }
        return result;
    }

    float SymbolDescriptor::toFloat(const std::vector<uint64_t>& offset) const
    {
        uint64_t addr = getConstLiteral(offset).getValue();
        
        return value_to_float_n(getValueAt(addr, cType.size() - 1), cType.back(), isSigned);
    }

    double SymbolDescriptor::toDouble(const std::vector<uint64_t>& offset) const
    {
        uint64_t addr = getConstLiteral(offset).getValue();

        return value_to_double_n(getValueAt(addr, cType.size() - 1), cType.back(), isSigned);
    }

    uint64_t SymbolDescriptor::toUnsigned(const std::vector<uint64_t>& offset) const
    {
        uint64_t addr = getConstLiteral(offset).getValue();

        return value_to_uint64_n(getValueAt(addr, cType.size() - 1), cType.back());
    }

    int64_t SymbolDescriptor::toSigned(const std::vector<uint64_t>& offset) const
    {
        uint64_t addr = getConstLiteral(offset).getValue();

        return value_to_int64_n(getValueAt(addr, cType.size() - 1), cType.back());
    }

    template <typename Op>
    SymbolDescriptor SymbolDescriptor::applyArithmetic(const SymbolDescriptor &right, Op op) const
    {
        SymbolDescriptor result;
        result.cType = cType;
        result.isSigned = (isSigned || right.isSigned);
        result.hasAddress = false;
        if (!cType.empty() && !right.cType.empty())
        {
            result.cType[0] = promoteType(cType[0], right.cType[0]);
        }
        else
        {
            result.cType.push_back(CType::Type::UNKNOWN);
        }
        switch (result.cType[0].type)
        {
        case CType::Type::FLOAT:
            result.value = static_cast<double>(op(toFloat(), right.toFloat()));
            break;
        case CType::Type::DOUBLE:
            result.value = static_cast<double>(op(toDouble(), right.toDouble()));
            break;
        default:
            {
                if (result.isSigned)
                {
                    result.value = (int64_t)op(isSigned ? toSigned() : toUnsigned(), right.isSigned ? right.toSigned() : right.toUnsigned());
                }
                else
                {
                    result.value = (uint64_t)op(isSigned ? toSigned() : toUnsigned(), right.isSigned ? right.toSigned() : right.toUnsigned());
                }
            }
            break;
        }
        return result;
    }

    template <typename Op>
    SymbolDescriptor SymbolDescriptor::applyComparison(const SymbolDescriptor &right, Op op) const
    {
        SymbolDescriptor result;
        result.cType = cType;
        result.isSigned = false;
        result.hasAddress = false;
        result.cType.clear();
        result.cType.push_back(CType::Type::BOOL);
        if (cType.empty())
        {
            return result;
        }
        switch (cType[0].type)
        {
        case CType::Type::FLOAT:
            result.value = static_cast<uint64_t>(op(toFloat(), right.toFloat()));
            break;
        case CType::Type::DOUBLE:
            result.value = static_cast<uint64_t>(op(toDouble(), right.toDouble()));
            break;
        default:
            result.value = static_cast<uint64_t>(isSigned ? op(toSigned(), right.toSigned()) : (op(toUnsigned(), right.toUnsigned())));
            break;
        }
        return result;
    }

    template <typename Op>
    SymbolDescriptor SymbolDescriptor::applyLogical(const SymbolDescriptor &right, Op op) const
    {
        SymbolDescriptor result;
        result.isSigned = false;
        result.hasAddress = false;
        result.cType.clear();
        result.cType.push_back(CType::Type::BOOL);
        if (cType.empty())
        {
            return result;
        }
        switch (cType[0].type)
        {
        case CType::Type::FLOAT:
            result.value = static_cast<uint64_t>(op(toFloat(), right.toFloat()));
            break;
        case CType::Type::DOUBLE:
            result.value = static_cast<uint64_t>(op(toDouble(), right.toDouble()));
            break;
        default:
            result.value = static_cast<uint64_t>(op(toUnsigned(), right.toUnsigned()));
            break;
        }
        return result;
    }

    template <typename Op>
    SymbolDescriptor SymbolDescriptor::applyBitwise(const SymbolDescriptor &right, Op op) const
    {
        SymbolDescriptor result;
        result.cType = cType;
        if (!cType.empty() && !right.cType.empty())
        {
            result.cType[0] = promoteType(cType[0], right.cType[0]);
        }
        else
        {
            result.cType.push_back(CType::Type::UNKNOWN);
        }
        result.isSigned = (isSigned || right.isSigned);
        result.hasAddress = false;
        result.value = op(toUnsigned(), right.toUnsigned());
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator+(const SymbolDescriptor &right) const
    {
        return applyArithmetic(right, std::plus<>());
    }

    SymbolDescriptor SymbolDescriptor::operator-(const SymbolDescriptor &right) const
    {
        return applyArithmetic(right, std::minus<>());
    }

    SymbolDescriptor SymbolDescriptor::operator-() const
    {
        return applyArithmetic(*this, [](auto a, auto)
                               { return -a; });
    }

    SymbolDescriptor SymbolDescriptor::operator*(const SymbolDescriptor &right) const
    {
        return applyArithmetic(right, std::multiplies<>());
    }

    SymbolDescriptor SymbolDescriptor::operator/(const SymbolDescriptor &right) const
    {
        return applyArithmetic(right, std::divides<>());
    }

    SymbolDescriptor SymbolDescriptor::operator%(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result;
        result.cType.push_back(CType::Type::INT);
        result.isSigned = isSigned;
        result.value = getValue();
        result.value = result.toUnsigned() % right.toUnsigned();
        result.hasAddress = false;
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator==(const SymbolDescriptor &right) const
    {
        return applyComparison(right, std::equal_to<>());
    }

    SymbolDescriptor SymbolDescriptor::operator!=(const SymbolDescriptor &right) const
    {
        return applyComparison(right, std::not_equal_to<>());
    }

    SymbolDescriptor SymbolDescriptor::operator<(const SymbolDescriptor &right) const
    {
        return applyComparison(right, std::less<>());
    }

    SymbolDescriptor SymbolDescriptor::operator>(const SymbolDescriptor &right) const
    {
        return applyComparison(right, std::greater<>());
    }

    SymbolDescriptor SymbolDescriptor::operator<=(const SymbolDescriptor &right) const
    {
        return applyComparison(right, std::less_equal<>());
    }

    SymbolDescriptor SymbolDescriptor::operator>=(const SymbolDescriptor &right) const
    {
        return applyComparison(right, std::greater_equal<>());
    }

    SymbolDescriptor SymbolDescriptor::operator&&(const SymbolDescriptor &right) const
    {
        return applyLogical(right, std::logical_and<>());
    }

    SymbolDescriptor SymbolDescriptor::operator||(const SymbolDescriptor &right) const
    {
        return applyLogical(right, std::logical_or<>());
    }

    SymbolDescriptor SymbolDescriptor::operator!() const
    {
        return applyLogical(*this, [](auto a, auto)
                            { return !a; });
    }

    SymbolDescriptor SymbolDescriptor::operator&(const SymbolDescriptor &right) const
    {
        return applyBitwise(right, std::bit_and<>());
    }

    SymbolDescriptor SymbolDescriptor::operator|(const SymbolDescriptor &right) const
    {
        return applyBitwise(right, std::bit_or<>());
    }

    SymbolDescriptor SymbolDescriptor::operator^(const SymbolDescriptor &right) const
    {
        return applyBitwise(right, std::bit_xor<>());
    }

    SymbolDescriptor SymbolDescriptor::operator~() const
    {
        SymbolDescriptor result;
        result.isSigned = isSigned;
        result.hasAddress = false;
        result.value = ~toUnsigned();
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator<<(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result;
        result.isSigned = isSigned;
        result.value = getValue();
        result.value = result.toUnsigned() << right.toUnsigned();
        result.hasAddress = false;
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator>>(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result;
        result.isSigned = isSigned;
        result.value = getValue();
        result.value = result.toUnsigned() >> right.toUnsigned();
        result.hasAddress = false;
        return result;
    }
} // namespace CdbgExpr