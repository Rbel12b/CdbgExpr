#include "CdbgExpr.h"
#include <sstream>
#include <bit>
#include <cstdint>
#include <iostream>
#include <vector>

namespace CdbgExpr
{
    DbgData *SymbolDescriptor::data = nullptr;
    bool SymbolDescriptor::assignmentAllowed = false;

    SymbolDescriptor SymbolDescriptor::strToNumber(const std::string &str)
    {
        SymbolDescriptor number;
        number.cType.push_back(CType::INT);
        number.value = std::stol(str);
        number.hasAddress = false;
        return number;
    }

    SymbolDescriptor SymbolDescriptor::dereference(int offset) const
    {
        if (cType.size() < 2 || (cType[0] != CType::POINTER && cType[0] != CType::ARRAY))
        {
            throw std::runtime_error("Cannot dereference a non-pointer type");
        }
        SymbolDescriptor result;
        result.cType = cType;
        result.cType.erase(result.cType.begin());
        result.value = getValue() + (offset * data->CTypeSize(result.cType[0]));
        result.hasAddress = true;
        return result;
    }

    SymbolDescriptor SymbolDescriptor::getMember(const std::string &name) const
    {
        auto it = std::find_if(members.begin(), members.end(), [name](const Member &member)
                               { return member.symbol->name == name; });
        if (it == members.end())
        {
            throw std::runtime_error("Member not found");
        }
        return *it->symbol;
    }

    SymbolDescriptor SymbolDescriptor::addressOf() const
    {
        size_t addr;
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
        result.cType.insert(result.cType.begin(), CType::POINTER);
        result.value = addr;
        result.hasAddress = false;
        return result;
    }

    void SymbolDescriptor::setValue(uint64_t val)
    {
        if (hasAddress)
        {
            for (int i = 0; i < data->CTypeSize(cType[0]); i++)
            {
                data->setByte(value + i, ((val >> (i * 8)) & 0xFF));
            }
        }
        else
        {
            value = val;
        }
    }

    uint64_t SymbolDescriptor::getValue() const
    {
        uint64_t val = 0;
        if (hasAddress)
        {
            for (int i = 0; i < data->CTypeSize(cType[0]); i++)
            {
                val |= data->getByte(value + i) << (i * 8);
            }
        }
        else
        {
            val = value;
        }
        return val;
    }

    std::string SymbolDescriptor::toString() const
    {
        if (cType.empty())
            return "<unknown type>";

        std::ostringstream result;

        result << "(";
        for (uint64_t i = 0; i < cType.size(); i++)
        {
            if (cType[i] == CType::POINTER)
            {
                result << "*";
            }
            else if (cType[i] == CType::STRUCT)
            {
                result << "struct ";
            }
            else if (cType[i] == CType::CHAR)
            {
                result << "char";
            }
            else if (cType[i] == CType::BOOL)
            {
                result << "bool";
            }
            else if (cType[i] == CType::SHORT)
            {
                result << "short";
            }
            else if (cType[i] == CType::INT)
            {
                result << "int";
            }
            else if (cType[i] == CType::LONG)
            {
                result << "long";
            }
            else if (cType[i] == CType::FLOAT)
            {
                result << "float";
            }
            else if (cType[i] == CType::DOUBLE)
            {
                result << "double";
            }
        }
        result << ") ";

        if (cType.size() > 1 && cType[0] == CType::POINTER)
        {
            if (cType[1] == CType::CHAR)
            {
                if (!getValue())
                {
                    return "0x0";
                }
                else
                {
                    result << "0x" << std::hex << reinterpret_cast<uint64_t>(getValue());
                }
                result << " \"";
                uint64_t addr = reinterpret_cast<uint64_t>(getValue());
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
                result << "0x" << std::hex << reinterpret_cast<uint64_t>(getValue());
                return result.str();
            }
        }

        CType baseType = cType.back();

        if (baseType == CType::INT)
        {
            return result.str() + std::to_string(getValue());
        }
        else if (baseType == CType::FLOAT)
        {
            return result.str() + std::to_string((float)getValue());
        }
        else if (baseType == CType::DOUBLE)
        {
            return result.str() + std::to_string((double)(getValue()));
        }
        else if (baseType == CType::BOOL)
        {
            return result.str() + ((bool)getValue() ? "true" : "false");
        }
        else if (baseType == CType::CHAR)
        {
            return result.str() + (char)getValue();
        }
        else if (baseType == CType::STRUCT)
        {
            result << "{ ";
            for (uint64_t i = 0; i < members.size(); i++)
            {
                if (i > 0)
                {
                    result << ", ";
                }
                result << members[i].symbol->name << " = " << members[i].symbol->toString();
            }
            result << " }";
            return result.str();
        }
        else if (baseType == CType::ARRAY)
        {
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
        float result = 0.0f;

        try
        {
            if (cType[0] == CType::FLOAT)
            {
                result = std::bit_cast<float>((uint32_t)getValue());
            }
            else if (cType[0] == CType::DOUBLE)
            {
                result = std::bit_cast<double>(getValue());
            }
            else if (isSigned && (getValue() & (0x01 << (data->CTypeSize(cType[0]) * 8 - 1))))
            {
                result = -1.0f * (float)getValue();
            }
            else
            {
                result = (float)getValue();
            }
        }
        catch (const std::out_of_range &e)
        {
            result = 0.0f;
        }
        return result;
    }

    double SymbolDescriptor::toDouble() const
    {
        double result = 0.0f;

        try
        {
            if (cType[0] == CType::FLOAT)
            {
                result = std::bit_cast<float>((uint32_t)getValue());
            }
            else if (cType[0] == CType::DOUBLE)
            {
                result = std::bit_cast<double>(getValue());
            }
            else if (isSigned && (getValue() & (0x01 << (data->CTypeSize(cType[0]) * 8 - 1))))
            {
                result = -1.0f * (double)getValue();
            }
            else
            {
                result = (double)getValue();
            }
        }
        catch (const std::out_of_range &e)
        {
            result = 0.0f;
        }
        return result;
    }

    int64_t SymbolDescriptor::toSigned() const
    {
        int64_t result = 0.0f;

        try
        {
            if (cType[0] == CType::FLOAT)
            {
                result = std::bit_cast<float>((uint32_t)getValue());
            }
            else if (cType[0] == CType::DOUBLE)
            {
                result = std::bit_cast<double>(getValue());
            }
            else if (isSigned && (getValue() & (0x01 << (data->CTypeSize(cType[0]) * 8 - 1))) && data->CTypeSize(cType[0]) != 8)
            {
                result = -(int64_t)getValue();
            }
            else
            {
                result = (int64_t)getValue();
            }
        }
        catch (const std::out_of_range &e)
        {
            result = 0.0f;
        }
        return result;
    }

    template <typename Op>
    SymbolDescriptor SymbolDescriptor::applyArithmetic(const SymbolDescriptor &right, Op op) const
    {
        SymbolDescriptor result;
        result.cType = cType;
        result.isSigned = isSigned;
        result.hasAddress = false;
        switch (cType[0])
        {
        case CType::FLOAT:
            result.value = std::bit_cast<uint32_t>(op(toFloat(), right.toFloat()));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<uint64_t>(op(toDouble(), right.toDouble()));
            break;
        default:
            result.value = isSigned ? (uint64_t)op(toSigned(), right.toSigned()) : (op(getValue(), right.getValue()));
            break;
        }
        return result;
    }

    template <typename Op>
    SymbolDescriptor SymbolDescriptor::applyComparison(const SymbolDescriptor &right, Op op) const
    {
        SymbolDescriptor result;
        result.cType = cType;
        result.isSigned = isSigned;
        result.hasAddress = false;
        result.cType.clear();
        result.cType.push_back(CType::BOOL);
        switch (cType[0])
        {
        case CType::FLOAT:
            result.value = std::bit_cast<char>(op(toFloat(), right.toFloat()));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<char>(op(toDouble(), right.toDouble()));
            break;
        default:
            result.value = std::bit_cast<char>(isSigned ? op(toSigned(), right.toSigned()) : (op(getValue(), right.getValue())));
            break;
        }
        return result;
    }

    template <typename Op>
    SymbolDescriptor SymbolDescriptor::applyLogical(const SymbolDescriptor &right, Op op) const
    {
        SymbolDescriptor result;
        result.isSigned = isSigned;
        result.hasAddress = false;
        result.cType.clear();
        result.cType.push_back(CType::BOOL);
        switch (cType[0])
        {
        case CType::FLOAT:
            result.value = std::bit_cast<char>(op(toFloat(), right.toFloat()));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<char>(op(toDouble(), right.toDouble()));
            break;
        default:
            result.value = std::bit_cast<bool>(op(getValue(), right.getValue()));
            break;
        }
        return result;
    }

    template <typename Op>
    SymbolDescriptor SymbolDescriptor::applyBitwise(const SymbolDescriptor &right, Op op) const
    {
        SymbolDescriptor result;
        result.cType = cType;
        result.isSigned = isSigned;
        result.hasAddress = false;
        result.value = op(getValue(), right.getValue());
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
        result.cType.push_back(CType::INT);
        result.isSigned = isSigned;
        result.value = getValue();
        result.value %= right.getValue();
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
        result.value = ~getValue();
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator<<(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result;
        result.isSigned = isSigned;
        result.value = getValue();
        result.value <<= right.getValue();
        result.hasAddress = false;
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator>>(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result;
        result.isSigned = isSigned;
        result.value = getValue();
        result.value >>= right.getValue();
        result.hasAddress = false;
        return result;
    }
} // namespace CdbgExpr