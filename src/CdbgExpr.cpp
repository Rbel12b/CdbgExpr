#include "CdbgExpr.h"
#include "SymbolDescriptor.h"
#include <sstream>
#include <bit>
#include <cstdint>
#include <iostream>
#include <stack>

namespace CdbgExpr
{
    SymbolDescriptor Expression::eval(bool assignmentAllowed)
    {
        tokenize(expr_);
        for (auto &token : tokens)
        {
            std::cout << "Token: " << (int)token.type << ", Value: " << token.value << "\n";
        }
        tokens = convertToPostfix(tokens); // Convert to postfix notation
        std::cout << "After conversion to postfix: \n";

        for (auto &token : tokens)
        {
            std::cout << "Token: " << (int)token.type << ", Value: " << token.value << "\n";
        }

        pos_ = 0;
        assignmentAllowed_ = assignmentAllowed;
        return parseExpression(); // Evaluate the expression
    }

    Expression::~Expression() {}

    void Expression::tokenize(const std::string &expr)
    {
        std::string token;
        Token currentToken;
        size_t i = 0;
        while (i < expr.size())
        {
            char c = expr[i];
            if (std::isspace(c))
            {
                i++;
                continue;
            }
            if (std::isdigit(c))
            {
                token.clear();
                while (i < expr.size() && std::isdigit(expr[i]))
                {
                    token += expr[i];
                    i++;
                }
                tokens.push_back({TokenType::NUMBER, token});
                continue;
            }
            if (std::isalpha(c) || c == '_')
            {
                token.clear();
                while (i < expr.size() && (std::isalnum(expr[i]) || expr[i] == '_'))
                {
                    token += expr[i];
                    i++;
                }
                tokens.push_back({TokenType::SYMBOL, token});
                continue;
            }
            if (c == '(' || c == ')')
            {
                tokens.push_back({TokenType::PARENTHESIS, std::string(1, c)});
                i++;
                continue;
            }
            if (c == '[' || c == ']')
            {
                tokens.push_back({TokenType::ARRAY_ACCESS, std::string(1, c)});
                i++;
                continue;
            }
            if (c == '.' || (c == '-' && i + 1 < expr.size() && expr[i + 1] == '>'))
            {
                if (c == '-')
                    i++;
                tokens.push_back({TokenType::STRUCT_ACCESS, std::string(1, c)});
                i++;
                continue;
            }
            token.clear();
            while (i < expr.size() && std::ispunct(expr[i]) && expr[i] != '(' && expr[i] != ')' && expr[i] != '[' && expr[i] != ']')
            {
                token += expr[i];
                i++;
            }
            tokens.push_back({TokenType::OPERATOR, token});
        }
    }

    std::vector<Expression::Token> Expression::convertToPostfix(const std::vector<Token> &tokens)
    {
        std::vector<Token> output;
        std::stack<Token> operators;

        for (const auto &token : tokens)
        {
            if (token.type == TokenType::SYMBOL || token.type == TokenType::NUMBER)
            {
                output.push_back(token);
            }
            else if (token.type == TokenType::PARENTHESIS)
            {
                if (token.value == "(")
                {
                    operators.push(token);
                }
                else if (token.value == ")")
                {
                    while (!operators.empty() && operators.top().value != "(")
                    {
                        output.push_back(operators.top());
                        operators.pop();
                    }
                    if (!operators.empty())
                        operators.pop();
                }
            }
            else if (token.type == TokenType::ARRAY_ACCESS)
            {
                if (token.value == "[")
                {
                    operators.push(token);
                }
                else if (token.value == "]")
                {
                    while (!operators.empty() && operators.top().value != "[")
                    {
                        output.push_back(operators.top());
                        operators.pop();
                    }
                    if (!operators.empty())
                        operators.pop();

                    output.push_back({TokenType::OPERATOR, "[]"});
                }
            }
            else if (token.type == TokenType::STRUCT_ACCESS)
            {
                while (!operators.empty() && getPrecedence(operators.top()) >= getPrecedence(token))
                {
                    output.push_back(operators.top());
                    operators.pop();
                }
                operators.push(token);
            }
            else if (token.type == TokenType::OPERATOR)
            {
                while (!operators.empty() && getPrecedence(operators.top()) >= getPrecedence(token))
                {
                    output.push_back(operators.top());
                    operators.pop();
                }
                operators.push(token);
            }
        }

        while (!operators.empty())
        {
            output.push_back(operators.top());
            operators.pop();
        }

        return output;
    }

    int Expression::getPrecedence(const Token &token)
    {
        if (token.type == TokenType::STRUCT_ACCESS)
            return 17; // . and ->
        if (token.value == "[]")
            return 16; // []
        if (token.value == "++" || token.value == "--")
            return 15;
        if (token.value == "*" || token.value == "/" || token.value == "%")
            return 14;
        if (token.value == "+" || token.value == "-")
            return 13;
        if (token.value == "<<" || token.value == ">>")
            return 12;
        if (token.value == "<" || token.value == "<=" || token.value == ">" || token.value == ">=")
            return 11;
        if (token.value == "==" || token.value == "!=")
            return 10;
        if (token.value == "&")
            return 9;
        if (token.value == "^")
            return 8;
        if (token.value == "|")
            return 7;
        if (token.value == "&&")
            return 6;
        if (token.value == "||")
            return 5;
        if (token.value == "?" || token.value == ":")
            return 4;
        if (token.value == "=" || token.value == "+=" || token.value == "-=" || token.value == "*=" || token.value == "/=" || token.value == "%=" ||
            token.value == "&=" || token.value == "^=" || token.value == "|=" || token.value == "<<=" || token.value == ">>=")
            return 3;
        if (token.value == ",")
            return 2;
        return 0;
    }

    SymbolDescriptor Expression::parseExpression()
    {
        std::stack<SymbolDescriptor> evalStack;

        for (const auto &token : tokens)
        {
            if (token.type == TokenType::SYMBOL)
            {
                evalStack.push(dbgData->getSymbol(token.value));
            }
            else if (token.type == TokenType::NUMBER)
            {
                evalStack.push(SymbolDescriptor::strToNumber(token.value)); // Convert to number
            }
            else if (token.type == TokenType::OPERATOR || token.type == TokenType::STRUCT_ACCESS)
            {
                if (evalStack.size() < 2)
                {
                    throw std::runtime_error("Insufficient operands for binary operator: " + token.value);
                }
                SymbolDescriptor right = evalStack.top();
                evalStack.pop();
                SymbolDescriptor left = evalStack.top();
                evalStack.pop();
                evalStack.push(parseBinaryOperator(left, right, token.value));
            }
        }

        if (evalStack.size() != 1)
        {
            throw std::runtime_error("Invalid expression evaluation");
        }

        return evalStack.top();
    }
    SymbolDescriptor Expression::parseBinaryOperator(const SymbolDescriptor &left, const SymbolDescriptor &right, const std::string &op)
    {
        if (op == "+")
        {
            return left + right;
        }
        else if (op == "-")
        {
            return left - right;
        }
        else if (op == "*")
        {
            return left * right;
        }
        else if (op == "/")
        {
            return left / right;
        }
        else if (op == "%")
        {
            return left % right;
        }
        else if (op == "&")
        {
            return left & right;
        }
        else if (op == "|")
        {
            return left | right;
        }
        else if (op == "^")
        {
            return left ^ right;
        }
        else if (op == "<<")
        {
            return left << right;
        }
        else if (op == ">>")
        {
            return left >> right;
        }
        else if (op == "&&")
        {
            return left && right;
        }
        else if (op == "||")
        {
            return left || right;
        }
        else if (op == "==")
        {
            return left == right;
        }
        else if (op == "!=")
        {
            return left != right;
        }
        else if (op == "<")
        {
            return left < right;
        }
        else if (op == "<=")
        {
            return left <= right;
        }
        else if (op == ">")
        {
            return left > right;
        }
        else if (op == ">=")
        {
            return left >= right;
        }
        else if (op == "[]")
        {
            return parseArrayAccess(left, right);
        }
        else if (op == "." || op == "->")
        {
            return parseMemberAccess(left, right.name, op == "->");
        }
        throw std::runtime_error("Unsupported binary operator: " + op);
    }

    SymbolDescriptor Expression::parseArrayAccess(const SymbolDescriptor &array, const SymbolDescriptor &index)
    {
        if (array.cType[0] != CType::POINTER && array.cType[0] != CType::ARRAY)
        {
            throw std::runtime_error("Cannot index a non-array type");
        }
        int idx = index.getValue();
        return array.dereference(idx);
    }

    SymbolDescriptor Expression::parseMemberAccess(const SymbolDescriptor &structOrPointer, const std::string &member, bool isPointerAccess)
    {
        if (isPointerAccess && structOrPointer.cType[0] != CType::POINTER)
        {
            throw std::runtime_error("Expected a pointer for '->' operator");
        }
        const SymbolDescriptor &baseStruct = isPointerAccess ? structOrPointer.dereference() : structOrPointer;
        return baseStruct.getMember(member);
    }

    // Symbol handling functions.

    DbgData *SymbolDescriptor::data = nullptr;

    SymbolDescriptor SymbolDescriptor::strToNumber(const std::string &str)
    {
        SymbolDescriptor number;
        number.cType.push_back(CType::INT);
        number.value = std::stol(str);
        number.hasAddress = false;
        return SymbolDescriptor();
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
                    return "0x0";
                result << "\"";
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

    SymbolDescriptor SymbolDescriptor::operator+(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        switch (result.cType[0])
        {
        case CType::FLOAT:
            result.value = std::bit_cast<uint32_t>(std::bit_cast<float>((uint32_t)result.getValue()) + std::bit_cast<float>((uint32_t)right.getValue()));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<uint64_t>(std::bit_cast<double>(result.getValue()) + std::bit_cast<double>(right.getValue()));
            break;
        default:
            result.value += right.getValue();
            break;
        }
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator-(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        switch (result.cType[0])
        {
        case CType::FLOAT:
            result.value = std::bit_cast<uint32_t>(std::bit_cast<float>((uint32_t)result.getValue()) - std::bit_cast<float>((uint32_t)right.getValue()));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<uint64_t>(std::bit_cast<double>(result.getValue()) - std::bit_cast<double>(right.getValue()));
            break;
        default:
            result.value -= right.getValue();
            break;
        }
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator*(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        switch (result.cType[0])
        {
        case CType::FLOAT:
            result.value = std::bit_cast<uint32_t>(std::bit_cast<float>((uint32_t)result.getValue()) * std::bit_cast<float>((uint32_t)right.getValue()));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<uint64_t>(std::bit_cast<double>(result.getValue()) * std::bit_cast<double>(right.getValue()));
            break;
        default:
            result.value *= right.getValue();
            break;
        }
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator/(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        switch (result.cType[0])
        {
        case CType::FLOAT:
            result.value = std::bit_cast<uint32_t>(std::bit_cast<float>((uint32_t)result.getValue()) / std::bit_cast<float>((uint32_t)right.getValue()));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<uint64_t>(std::bit_cast<double>(result.getValue()) / std::bit_cast<double>(right.getValue()));
            break;
        default:
            result.value /= right.getValue();
            break;
        }
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator%(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        result.value %= right.getValue();
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator==(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        switch (result.cType[0])
        {
        case CType::FLOAT:
            result.value = std::bit_cast<char>(std::bit_cast<float>((uint32_t)result.getValue()) == std::bit_cast<float>((uint32_t)right.getValue()));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<char>(std::bit_cast<double>(result.getValue()) == std::bit_cast<double>(right.getValue()));
            break;
        default:
            result.value = (bool)(result.getValue() == right.getValue());
            break;
        }
        result.cType.clear();
        result.cType.push_back(CType::BOOL);
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator!=(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        switch (result.cType[0])
        {
        case CType::FLOAT:
            result.value = std::bit_cast<char>(std::bit_cast<float>((uint32_t)result.getValue()) != std::bit_cast<float>((uint32_t)right.getValue()));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<char>(std::bit_cast<double>(result.getValue()) != std::bit_cast<double>(right.getValue()));
            break;
        default:
            result.value = (bool)(result.getValue() != right.getValue());
            break;
        }
        result.cType.clear();
        result.cType.push_back(CType::BOOL);
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator<(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        switch (result.cType[0])
        {
        case CType::FLOAT:
            result.value = std::bit_cast<char>(std::bit_cast<float>((uint32_t)result.getValue()) < std::bit_cast<float>((uint32_t)right.getValue()));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<char>(std::bit_cast<double>(result.getValue()) < std::bit_cast<double>(right.getValue()));
            break;
        default:
            result.value = (bool)(result.getValue() < right.getValue());
            break;
        }
        result.cType.clear();
        result.cType.push_back(CType::BOOL);
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator>(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        switch (result.cType[0])
        {
        case CType::FLOAT:
            result.value = std::bit_cast<char>(std::bit_cast<float>((uint32_t)result.getValue()) > std::bit_cast<float>((uint32_t)right.getValue()));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<char>(std::bit_cast<double>(result.getValue()) > std::bit_cast<double>(right.getValue()));
            break;
        default:
            result.value = (bool)(result.getValue() > right.getValue());
            break;
        }
        result.cType.clear();
        result.cType.push_back(CType::BOOL);
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator<=(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        switch (result.cType[0])
        {
        case CType::FLOAT:
            result.value = std::bit_cast<char>(std::bit_cast<float>((uint32_t)result.getValue()) <= std::bit_cast<float>((uint32_t)right.getValue()));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<char>(std::bit_cast<double>(result.getValue()) <= std::bit_cast<double>(right.getValue()));
            break;
        default:
            result.value = (bool)(result.getValue() <= right.getValue());
            break;
        }
        result.cType.clear();
        result.cType.push_back(CType::BOOL);
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator>=(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        switch (result.cType[0])
        {
        case CType::FLOAT:
            result.value = std::bit_cast<char>(std::bit_cast<float>((uint32_t)result.getValue()) >= std::bit_cast<float>((uint32_t)right.getValue()));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<char>(std::bit_cast<double>(result.getValue()) >= std::bit_cast<double>(right.getValue()));
            break;
        default:
            result.value = (bool)(result.getValue() >= right.getValue());
            break;
        }
        result.cType.clear();
        result.cType.push_back(CType::BOOL);
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator&&(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        result.value = (bool)result.getValue() && (bool)right.getValue();
        result.cType.clear();
        result.cType.push_back(CType::BOOL);
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator||(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        result.value = (bool)result.getValue() || (bool)right.getValue();
        result.cType.clear();
        result.cType.push_back(CType::BOOL);
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator!() const
    {
        SymbolDescriptor result = *this;
        result.value = !((bool)result.getValue());
        result.cType.clear();
        result.cType.push_back(CType::BOOL);
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator&(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        result.value &= right.getValue();
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator|(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        result.value &= right.getValue();
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator^(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        result.value &= right.getValue();
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator~() const
    {
        SymbolDescriptor result = *this;
        result.value = ~result.getValue();
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator<<(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        result.value <<= right.getValue();
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator>>(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        result.value >>= right.getValue();
        return result;
    }
} // namespace CdbgExpr