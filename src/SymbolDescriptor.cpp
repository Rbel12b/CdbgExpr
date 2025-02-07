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

    uint64_t SymbolDescriptor::value_to_uint64_b(const std::variant<uint64_t, int64_t, double> &v)
    {
        if (std::holds_alternative<uint64_t>(v)) {
            return std::get<uint64_t>(v);
        } else if (std::holds_alternative<int64_t>(v)) {
            return static_cast<uint64_t>(std::get<int64_t>(v));
        } else if (std::holds_alternative<double>(v)) {
            return std::bit_cast<uint64_t>(std::get<double>(v));  // Binary representation
        }
        return 0;
    }

    uint64_t SymbolDescriptor::value_to_uint64_n(const std::variant<uint64_t, int64_t, double> &v)
    {
        if (std::holds_alternative<uint64_t>(v)) {
            return std::get<uint64_t>(v);
        } else if (std::holds_alternative<int64_t>(v)) {
            return static_cast<uint64_t>(std::get<int64_t>(v));
        } else if (std::holds_alternative<double>(v)) {
            return static_cast<uint64_t>(std::get<double>(v));  // Binary representation
        }
        return 0;
    }

    int64_t SymbolDescriptor::value_to_int64_b(const std::variant<uint64_t, int64_t, double> &v)
    {
        if (std::holds_alternative<uint64_t>(v)) {
            return std::get<uint64_t>(v);
        } else if (std::holds_alternative<int64_t>(v)) {
            return static_cast<uint64_t>(std::get<int64_t>(v));
        } else if (std::holds_alternative<double>(v)) {
            return std::bit_cast<uint64_t>(std::get<double>(v));  // Binary representation
        }
        return 0;
    }

    int64_t SymbolDescriptor::value_to_int64_n(const std::variant<uint64_t, int64_t, double> &v)
    {
        if (std::holds_alternative<uint64_t>(v)) {
            return std::get<uint64_t>(v);
        } else if (std::holds_alternative<int64_t>(v)) {
            return static_cast<uint64_t>(std::get<int64_t>(v));
        } else if (std::holds_alternative<double>(v)) {
            return static_cast<uint64_t>(std::get<double>(v));  // Binary representation
        }
        return 0;
    }
    double SymbolDescriptor::value_to_double_b(const std::variant<uint64_t, int64_t, double> &v)
    {
        if (std::holds_alternative<uint64_t>(v)) {
            return std::bit_cast<double>(std::get<uint64_t>(v));
        } else if (std::holds_alternative<int64_t>(v)) {
            return std::bit_cast<double>(std::get<int64_t>(v));
        } else if (std::holds_alternative<double>(v)) {
            return std::get<double>(v);  // Binary representation
        }
        return 0;
    }
    double SymbolDescriptor::value_to_double_n(const std::variant<uint64_t, int64_t, double> &v)
    {
        if (std::holds_alternative<uint64_t>(v)) {
            return static_cast<double>(std::get<uint64_t>(v));
        } else if (std::holds_alternative<int64_t>(v)) {
            return static_cast<double>(std::get<int64_t>(v));
        } else if (std::holds_alternative<double>(v)) {
            return std::get<double>(v);  // Binary representation
        }
        return 0;
    }

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
        return *(it->second.symbol);
    }

    SymbolDescriptor SymbolDescriptor::addressOf() const
    {
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
        result.cType.insert(result.cType.begin(), CType::POINTER);
        result.value = addr;
        result.hasAddress = false;
        return result;
    }

    void SymbolDescriptor::setValue(const std::variant<uint64_t, int64_t, double>& val)
    {
        if (hasAddress)
        {
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

    std::variant<uint64_t, int64_t, double> SymbolDescriptor::getValue() const
    {
        uint64_t val = 0;
        if (hasAddress)
        {
            for (int i = 0; i < data->CTypeSize(cType[0]); i++)
            {
                val |= data->getByte(value_to_uint64_b(value) + i) << (i * 8);
            }
        }
        else
        {
            val = value_to_uint64_b(value);
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

        if (baseType == CType::STRUCT)
        {
            result << "{ ";
            for (auto& member : members)
            {
                result << member.first << " = ";
                result << member.second.symbol->toString();
                result << ", ";
            }
            result << " }";
            return result.str();
        }
        else if (baseType == CType::ARRAY)
        {
        }
        else
        {
            return std::visit([](auto && value) { return std::to_string(value); }, getValue());
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
        return static_cast<float>(value_to_double_n(this->getValue()));
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
        result.isSigned = isSigned;
        result.hasAddress = false;
        switch (cType[0])
        {
        case CType::FLOAT:
            result.value = static_cast<uint64_t>(std::bit_cast<uint32_t>(op(toFloat(), right.toFloat())));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<uint64_t>(op(toDouble(), right.toDouble()));
            break;
        default:
            result.value = isSigned ? (uint64_t)op(toSigned(), right.toSigned()) : (op(toUnsigned(), right.toUnsigned()));
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
            result.value = std::bit_cast<char>(isSigned ? op(toSigned(), right.toSigned()) : (op(toUnsigned(), right.toUnsigned())));
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
            result.value = static_cast<uint64_t>(op(toFloat(), right.toFloat()));
            break;
        case CType::DOUBLE:
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
        result.isSigned = isSigned;
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
        result.cType.push_back(CType::INT);
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