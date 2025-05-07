#include "CdbgExpr.h"
#include <sstream>
#include <bit>
#include <cstdint>
#include <iostream>
#include <vector>
#include "SymbolDescriptor.h"

namespace CdbgExpr
{
    DbgData *SymbolDescriptor::data = nullptr;
    bool SymbolDescriptor::assignmentAllowed = false;

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

    uint64_t SymbolDescriptor::value_to_uint64_b(const std::variant<uint64_t, int64_t, double, float> &v)
    {
        if (std::holds_alternative<uint64_t>(v)) {
            return std::get<uint64_t>(v);
        } else if (std::holds_alternative<int64_t>(v)) {
            return std::bit_cast<uint64_t>(std::get<int64_t>(v));
        } else if (std::holds_alternative<double>(v)) {
            return std::bit_cast<uint64_t>(std::get<double>(v));
        } else if (std::holds_alternative<float>(v)) {
            return std::bit_cast<uint32_t>(std::get<float>(v));
        }
        return 0;
    }

    uint64_t SymbolDescriptor::value_to_uint64_n(const std::variant<uint64_t, int64_t, double, float> &v)
    {
        if (std::holds_alternative<uint64_t>(v)) {
            return std::get<uint64_t>(v);
        } else if (std::holds_alternative<int64_t>(v)) {
            return static_cast<uint64_t>(std::get<int64_t>(v));
        } else if (std::holds_alternative<double>(v)) {
            return static_cast<uint64_t>(std::get<double>(v));
        } else if (std::holds_alternative<float>(v)) {
            return static_cast<uint64_t>(std::get<float>(v));
        }
        return 0;
    }

    int64_t SymbolDescriptor::value_to_int64_b(const std::variant<uint64_t, int64_t, double, float> &v)
    {
        if (std::holds_alternative<uint64_t>(v)) {
            return std::bit_cast<int64_t>(std::get<uint64_t>(v));
        } else if (std::holds_alternative<int64_t>(v)) {
            return std::get<int64_t>(v);
        } else if (std::holds_alternative<double>(v)) {
            return std::bit_cast<int64_t>(std::get<double>(v));
        } else if (std::holds_alternative<float>(v)) {
            return std::bit_cast<uint32_t>(std::get<float>(v));
        }
        return 0;
    }

    int64_t SymbolDescriptor::value_to_int64_n(const std::variant<uint64_t, int64_t, double, float> &v)
    {
        if (std::holds_alternative<uint64_t>(v)) {
            return static_cast<int64_t>(std::get<uint64_t>(v));
        } else if (std::holds_alternative<int64_t>(v)) {
            return std::get<int64_t>(v);
        } else if (std::holds_alternative<double>(v)) {
            return static_cast<int64_t>(std::get<double>(v));
        } else if (std::holds_alternative<float>(v)) {
            return static_cast<int64_t>(std::get<float>(v));
        }
        return 0;
    }
    double SymbolDescriptor::value_to_double_b(const std::variant<uint64_t, int64_t, double, float> &v)
    {
        if (std::holds_alternative<uint64_t>(v)) {
            return std::bit_cast<double>(std::get<uint64_t>(v));
        } else if (std::holds_alternative<int64_t>(v)) {
            return std::bit_cast<double>(std::get<int64_t>(v));
        } else if (std::holds_alternative<double>(v)) {
            return std::get<double>(v);
        } else if (std::holds_alternative<float>(v)) {
            return std::get<float>(v);
        }
        return 0.0f;
    }
    double SymbolDescriptor::value_to_double_n(const std::variant<uint64_t, int64_t, double, float> &v)
    {
        if (std::holds_alternative<uint64_t>(v)) {
            return static_cast<double>(std::get<uint64_t>(v));
        } else if (std::holds_alternative<int64_t>(v)) {
            return static_cast<double>(std::get<int64_t>(v));
        } else if (std::holds_alternative<double>(v)) {
            return std::get<double>(v);
        } else if (std::holds_alternative<float>(v)) {
            return std::get<float>(v);
        }
        return 0.0f;
    }

    float SymbolDescriptor::value_to_float_b(const std::variant<uint64_t, int64_t, double, float> &v)
    {
        if (std::holds_alternative<uint64_t>(v)) {
            return std::bit_cast<float>((uint32_t)std::get<uint64_t>(v));
        } else if (std::holds_alternative<int64_t>(v)) {
            return std::bit_cast<float>((uint32_t)std::get<int64_t>(v));
        } else if (std::holds_alternative<double>(v)) {
            return std::bit_cast<float>((uint32_t)std::bit_cast<uint64_t>(std::get<double>(v)));
        } else if (std::holds_alternative<float>(v)) {
            return std::get<float>(v);
        }
        return 0.0f;
    }

    float SymbolDescriptor::value_to_float_n(const std::variant<uint64_t, int64_t, double, float> &v)
    {
        if (std::holds_alternative<uint64_t>(v)) {
            return static_cast<float>(std::get<uint64_t>(v));
        } else if (std::holds_alternative<int64_t>(v)) {
            return static_cast<float>(std::get<int64_t>(v));
        } else if (std::holds_alternative<double>(v)) {
            return std::get<double>(v);
        } else if (std::holds_alternative<float>(v)) {
            return std::get<float>(v);
        }
        return 0.0f;
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

        if (s.empty())
        {
            cType.push_back(CType::Type::INT);
            value = (int64_t)0;
            return;
        }
        
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

        char* endptr;
        if (s.find('.') != std::string::npos ||
            s.find('e') != std::string::npos || s.find('E') != std::string::npos)
        {
            double d = std::strtod(s.c_str(), &endptr);
            cType.push_back(CType::Type::DOUBLE);
            value = d;
        }
        else
        {
            if (s[0] == '-')
            {
                isSigned = true;
            }
            uint64_t i = std::strtoull(s.c_str(), &endptr, 0);
            cType.push_back(CType::Type::LONGLONG);
            if (isSigned)
            {
                i = uint64_t(-(int64_t(i)));
            }
            value = i;
            return;
        }
        cType.push_back(CType::Type::INT);
        value = (int64_t)0;
        return;
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
        if (data == nullptr)
        {
            throw std::runtime_error("DbgData pointer is null");
        }
        if (cType.size() < 2 || (cType[0] != CType::Type::POINTER && cType[0] != CType::Type::ARRAY))
        {
            throw std::runtime_error("Cannot dereference a non-pointer type");
        }
        SymbolDescriptor result;
        result.cType = cType;
        result.cType.erase(result.cType.begin());
        result.value = value_to_uint64_n(getValue()) + (offset * data->CTypeSize(result.cType[0]));
        result.hasAddress = true;
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
            addr = value_to_uint64_b(value);
        }
        SymbolDescriptor result;
        result.cType = cType;
        result.cType.insert(result.cType.begin(), CType::Type::POINTER);
        result.value = addr;
        result.hasAddress = false;
        return result;
    }

    void SymbolDescriptor::setValue(const std::variant<uint64_t, int64_t, double, float>& val)
    {
        if (data == nullptr)
        {
            throw std::runtime_error("DbgData pointer is null");
        }
        if (hasAddress)
        {
            if (cType.empty())
            {
                return;
            }
            uint64_t newValue = value_to_uint64_b(val);
            for (int i = 0; i < data->CTypeSize(cType[0]); i++)
            {
                data->setByte(value_to_uint64_b(value) + i, ((newValue >> (i * 8)) & 0xFF));
            }
        }
        else
        {
            value = val;
        }
    }

    std::variant<uint64_t, int64_t, double, float> SymbolDescriptor::getValue() const
    {
        if (data == nullptr)
        {
            throw std::runtime_error("DbgData pointer is null");
        }
        uint64_t val = 0;
        if (hasAddress)
        {
            if (cType.empty())
            {
                return 0;
            }
            for (int i = 0; i < data->CTypeSize(cType[0]); i++)
            {
                val |= data->getByte(value_to_uint64_b(value) + i) << (i * 8);
            }
            return val;
        }
        else
        {
            return value;
        }
        return val;
    }

    std::string SymbolDescriptor::toString() const
    {
        if (data == nullptr)
        {
            throw std::runtime_error("DbgData pointer is null");
        }
        if (cType.empty())
            return "<unknown type>";

        std::ostringstream result;

        result << "(";
        uint64_t i = 0;
        while (i < cType.size() && cType[i] == CType::Type::POINTER)
        {
            result << "*";
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
            result << "unsigned";
        }
        for (; i < cType.size(); i++)
        {
            result << " ";
            if (cType[i] == CType::Type::POINTER)
            {
                result << "*";
            }
            else if (cType[i] == CType::Type::VOID)
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
        result << ") ";

        if (cType.size() > 1 && cType[0] == CType::Type::POINTER)
        {
            if (cType[1] == CType::Type::CHAR)
            {
                if (!value_to_uint64_b(getValue()))
                {
                    return "0x0";
                }
                else
                {
                    result << "0x" << std::hex << value_to_uint64_b(getValue());
                }
                result << " \"";
                uint64_t addr = value_to_uint64_b(getValue());
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
                result << "0x" << std::hex << value_to_uint64_b(getValue());
                return result.str();
            }
        }

        CType baseType = cType.back();

        if (baseType == CType::Type::STRUCT)
        {
            result << "{ ";
            for (auto& member : members)
            {
                result << member.first << " = ";
                result << member.second.symbol.toString();
                result << ", ";
            }
            result << " }";
            return result.str();
        }
        else if (baseType == CType::Type::ARRAY)
        {
        }
        else
        {
            return result.str() + std::visit([](auto && value) { return std::to_string(value); }, getValue());
        }

        return result.str() + "<unsupported type>";
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

    float SymbolDescriptor::toFloat() const
    {
        return static_cast<float>(value_to_float_n(this->getValue()));
    }

    double SymbolDescriptor::toDouble() const
    {
        return static_cast<double>(value_to_double_n(this->getValue()));
    }

    uint64_t SymbolDescriptor::toUnsigned() const
    {
        return static_cast<uint64_t>(value_to_uint64_n(this->getValue()));
    }

    int64_t SymbolDescriptor::toSigned() const
    {
        return static_cast<int64_t>(value_to_int64_n(this->getValue()));
    }

    template <typename Op>
    SymbolDescriptor SymbolDescriptor::applyArithmetic(const SymbolDescriptor &right, Op op) const
    {
        SymbolDescriptor result;
        result.cType = cType;
        result.isSigned = (isSigned || right.isSigned);
        result.hasAddress = false;
        if (!cType.empty())
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
        if (!cType.empty())
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