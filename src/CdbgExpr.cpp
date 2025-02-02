#include "CdbgExpr.h"
#include "SymbolDescriptor.h"
#include <sstream>
#include <bit>
#include <cstdint>
#include <iostream>

namespace CdbgExpr
{

    SymbolDescriptor Expression::eval(bool assignmentAllowed)
    {
        tokenize(expr_);
        for (auto &token : tokens)
        {
            std::cout << "Token: " << (int)token.type << ", Value: " << token.value << "\n";
        }
        pos_ = 0;
        assignmentAllowed_ = assignmentAllowed;
        SymbolDescriptor result = parseExpression();
        return result;
    }

    Expression::~Expression() {}

    void Expression::tokenize(const std::string &expr)
    {
        std::string token;
        Token currentToken;
        for (char c : expr)
        {
            if (c == ' ' || c == '\t' || c == '\n')
            {
                if (!token.empty())
                {
                    currentToken.type = TokenType::SYMBOL;
                    currentToken.value = token;
                    tokens.push_back(currentToken);
                    token.clear();
                }
            }
            else if (c == '(' || c == '[')
            {
                if (!token.empty())
                {
                    currentToken.type = TokenType::SYMBOL;
                    currentToken.value = token;
                    tokens.push_back(currentToken);
                    token.clear();
                }
                currentToken.type = TokenType::PARENTHESIS;
                currentToken.value = std::string(1, c);
                tokens.push_back(currentToken);
                currentToken.children = {};
                tokenizeNested(expr, currentToken);
            }
            else if (c == ')' || c == ']')
            {
                if (!token.empty())
                {
                    currentToken.type = TokenType::SYMBOL;
                    currentToken.value = token;
                    tokens.back().children.push_back(currentToken);
                    token.clear();
                }
                currentToken.type = TokenType::PARENTHESIS;
                currentToken.value = std::string(1, c);
                tokens.back().children.push_back(currentToken);
            }
            else if (c == '.' || c == '->')
            {
                if (!token.empty())
                {
                    currentToken.type = TokenType::SYMBOL;
                    currentToken.value = token;
                    tokens.push_back(currentToken);
                    token.clear();
                }
                currentToken.type = TokenType::STRUCT_ACCESS;
                currentToken.value = std::string(1, c);
                tokens.push_back(currentToken);
            }
            else if (c == '=' || c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
                     c == '!' || c == '&' || c == '|' || c == '^' || c == '~' || c == '<' ||
                     c == '>' || c == '(' || c == ')' || c == '[' || c == ']' || c == '{' ||
                     c == '}' || c == ',' || c == ';' || c == '?' || c == ':')
            {
                if (!token.empty())
                {
                    currentToken.type = TokenType::SYMBOL;
                    currentToken.value = token;
                    tokens.push_back(currentToken);
                    token.clear();
                }
                currentToken.type = TokenType::OPERATOR;
                currentToken.value = std::string(1, c);
                tokens.push_back(currentToken);
            }
            else
            {
                token += c;
            }
        }
        if (!token.empty())
        {
            currentToken.type = TokenType::SYMBOL;
            currentToken.value = token;
            tokens.push_back(currentToken);
        }
    }
    
    void Expression::tokenizeNested(const std::string& expr, Token& token) {
        std::string nestedExpr;
        int depth = 1;
        for (char c : expr) {
            if (c == '(' || c == '[') {
                depth++;
            } else if (c == ')' || c == ']') {
                depth--;
                if (depth == 0) {
                    break;
                }
            }
            nestedExpr += c;
        }
        tokenize(nestedExpr, token.children);
    }

    SymbolDescriptor Expression::parseExpression()
    {
        if (tokens.empty())
        {
            throw std::runtime_error("Unexpected end of expression");
        }
        Token token = tokens.front();
        tokens.erase(tokens.begin());
        if (token.type == TokenType::PARENTHESIS)
        {
            if (token.value == "(")
            {
                SymbolDescriptor expr = parseExpression(token.children);
                if (token.children.empty() || token.children.front().type != TokenType::PARENTHESIS ||
                    token.children.front().value != ")")
                {
                    throw std::runtime_error("Unbalanced parentheses");
                }
                token.children.erase(token.children.begin());
                return expr;
            }
            else
            {
                throw std::runtime_error("Unexpected closing parenthesis");
            }
        }
        else if (token.type == TokenType::SYMBOL)
        {
            SymbolDescriptor symbol = dbgData->getSymbol(token.value);
            if (!tokens.empty() && tokens.front().type == TokenType::ARRAY_ACCESS)
            {
                return parseArrayAccess(symbol, tokens.front().children);
            }
            else if (!tokens.empty() && tokens.front().type == TokenType::STRUCT_ACCESS)
            {
                return parseStructAccess(symbol, tokens.front().children);
            }
            else
            {
                return symbol;
            }
        }
        else if (token.type == TokenType::NUMBER)
        {
            // Handle number literals
            // ...
        }
        else
        {
            throw std::runtime_error("Unexpected token");
        }
    }

    SymbolDescriptor Expression::parseExpression(std::vector<Token> &tokens)
    {
        SymbolDescriptor left = parseTerm(tokens);
        while (true)
        {
            if (tokens.empty())
            {
                break;
            }
            Token token = tokens.front();
            tokens.erase(tokens.begin());
            if (token.type == TokenType::OPERATOR)
            {
                SymbolDescriptor right = parseTerm(tokens);
                left = parseBinaryOperator(left, right);
            }
            else
            {
                break;
            }
        }
        return left;
    }

    SymbolDescriptor Expression::parseTerm(std::vector<Token> &tokens)
    {
        SymbolDescriptor left = parseFactor(tokens);
        while (true)
        {
            if (tokens.empty())
            {
                break;
            }
            Token token = tokens.front();
            tokens.erase(tokens.begin());
            if (token.type == TokenType::OPERATOR)
            {
                SymbolDescriptor right = parseFactor(tokens);
                left = parseBinaryOperator(left, right);
            }
            else
            {
                break;
            }
        }
        return left;
    }

    SymbolDescriptor Expression::parseFactor(std::vector<Token> &tokens)
    {
        if (tokens.empty())
        {
            throw std::runtime_error("Unexpected end of expression");
        }
        Token token = tokens.front();
        tokens.erase(tokens.begin());
        if (token.type == TokenType::SYMBOL)
        {
            SymbolDescriptor symbol = dbgData->getSymbol(token.value);
            if (!tokens.empty() && tokens.front().type == TokenType::ARRAY_ACCESS)
            {
                return parseArrayAccess(symbol, tokens);
            }
            else if (!tokens.empty() && tokens.front().type == TokenType::STRUCT_ACCESS)
            {
                return parseStructAccess(symbol, tokens);
            }
            else
            {
                return symbol;
            }
        }
        else if (token.type == TokenType::NUMBER)
        {
            // Handle number literals
            // ...
        }
        else if (token.type == TokenType::PARENTHESIS)
        {
            if (token.value == "(")
            {
                SymbolDescriptor expr = parseExpression(token.children);
                if (token.children.empty() || token.children.front().type != TokenType::PARENTHESIS ||
                    token.children.front().value != ")")
                {
                    throw std::runtime_error("Unbalanced parentheses");
                }
                token.children.erase(token.children.begin());
                return expr;
            }
            else
            {
                throw std::runtime_error("Unexpected closing parenthesis");
            }
        }
        else
        {
            throw std::runtime_error("Unexpected token");
        }
    }

    SymbolDescriptor Expression::parseArrayAccess(SymbolDescriptor symbol, std::vector<Token> &tokens)
    {
        if (tokens.empty() || tokens.front().type != TokenType::ARRAY_ACCESS ||
            tokens.front().value != "[")
        {
            throw std::runtime_error("Expected array access operator");
        }
        tokens.erase(tokens.begin());
        SymbolDescriptor index = parseExpression(tokens);
        if (tokens.empty() || tokens.front().type != TokenType::ARRAY_ACCESS ||
            tokens.front().value != "]")
        {
            throw std::runtime_error("Expected closing array access operator");
        }
        tokens.erase(tokens.begin());
        // Handle array access
        // ...
        return symbol;
    }

    SymbolDescriptor Expression::parseStructAccess(SymbolDescriptor symbol, std::vector<Token> &tokens)
    {
        if (tokens.empty() || tokens.front().type != TokenType::STRUCT_ACCESS)
        {
            throw std::runtime_error("Expected struct access operator");
        }
        Token token = tokens.front();
        tokens.erase(tokens.begin());
        SymbolDescriptor member = dbgData->getSymbol(token.value);
        // Handle struct access
        // ...
        return symbol;
    }

    SymbolDescriptor Expression::parseUnaryOperator(SymbolDescriptor symbol)
    {
        if (tokens.empty() || tokens.front().type != TokenType::OPERATOR)
        {
            throw std::runtime_error("Expected unary operator");
        }
        Token token = tokens.front();
        tokens.erase(tokens.begin());
        // Handle unary operator
        // ...
        return symbol;
    }

    SymbolDescriptor Expression::parseBinaryOperator(SymbolDescriptor left, SymbolDescriptor right)
    {
        if (tokens.empty() || tokens.front().type != TokenType::OPERATOR)
        {
            throw std::runtime_error("Expected binary operator");
        }
        Token token = tokens.front();
        tokens.erase(tokens.begin());
        // Handle binary operator
        // ...
        return left;
    }

    // Symbol handling functions.

    void SymbolDescriptor::setValue(uint64_t val, DbgData *data)
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

    uint64_t SymbolDescriptor::getValue(DbgData *data)
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

    std::string SymbolDescriptor::toString(std::function<uint8_t(uint64_t)> memoryReader) const
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
                if (!value)
                    return "0x0";
                result << "\"";
                uint64_t addr = reinterpret_cast<uint64_t>(value);
                char ch;
                while ((ch = static_cast<char>(memoryReader(addr++))) != '\0')
                {
                    result << ch;
                }
                result << "\"";
                return result.str();
            }
            else
            {
                result << "0x" << std::hex << reinterpret_cast<uint64_t>(value);
                return result.str();
            }
        }

        CType baseType = cType.back();

        if (baseType == CType::INT)
        {
            return result.str() + std::to_string(value);
        }
        else if (baseType == CType::FLOAT)
        {
            return result.str() + std::to_string((float)value);
        }
        else if (baseType == CType::DOUBLE)
        {
            return result.str() + std::to_string((double)(value));
        }
        else if (baseType == CType::BOOL)
        {
            return result.str() + ((bool)value ? "true" : "false");
        }
        else if (baseType == CType::CHAR)
        {
            return result.str() + (char)value;
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
                result << members[i].symbol->name << " = " << members[i].symbol->toString(memoryReader);
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
            result.value = std::bit_cast<uint32_t>(std::bit_cast<float>((uint32_t)result.value) + std::bit_cast<float>((uint32_t)right.value));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<uint64_t>(std::bit_cast<double>(result.value) + std::bit_cast<double>(right.value));
            break;
        default:
            result.value += right.value;
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
            result.value = std::bit_cast<uint32_t>(std::bit_cast<float>((uint32_t)result.value) - std::bit_cast<float>((uint32_t)right.value));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<uint64_t>(std::bit_cast<double>(result.value) - std::bit_cast<double>(right.value));
            break;
        default:
            result.value -= right.value;
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
            result.value = std::bit_cast<uint32_t>(std::bit_cast<float>((uint32_t)result.value) * std::bit_cast<float>((uint32_t)right.value));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<uint64_t>(std::bit_cast<double>(result.value) * std::bit_cast<double>(right.value));
            break;
        default:
            result.value *= right.value;
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
            result.value = std::bit_cast<uint32_t>(std::bit_cast<float>((uint32_t)result.value) / std::bit_cast<float>((uint32_t)right.value));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<uint64_t>(std::bit_cast<double>(result.value) / std::bit_cast<double>(right.value));
            break;
        default:
            result.value /= right.value;
            break;
        }
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator%(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        result.value %= right.value;
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator==(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        switch (result.cType[0])
        {
        case CType::FLOAT:
            result.value = std::bit_cast<char>(std::bit_cast<float>((uint32_t)result.value) == std::bit_cast<float>((uint32_t)right.value));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<char>(std::bit_cast<double>(result.value) == std::bit_cast<double>(right.value));
            break;
        default:
            result.value = (bool)(result.value == right.value);
            break;
        }
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator!=(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        switch (result.cType[0])
        {
        case CType::FLOAT:
            result.value = std::bit_cast<char>(std::bit_cast<float>((uint32_t)result.value) != std::bit_cast<float>((uint32_t)right.value));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<char>(std::bit_cast<double>(result.value) != std::bit_cast<double>(right.value));
            break;
        default:
            result.value = (bool)(result.value != right.value);
            break;
        }
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator<(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        switch (result.cType[0])
        {
        case CType::FLOAT:
            result.value = std::bit_cast<char>(std::bit_cast<float>((uint32_t)result.value) < std::bit_cast<float>((uint32_t)right.value));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<char>(std::bit_cast<double>(result.value) < std::bit_cast<double>(right.value));
            break;
        default:
            result.value = (bool)(result.value < right.value);
            break;
        }
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator>(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        switch (result.cType[0])
        {
        case CType::FLOAT:
            result.value = std::bit_cast<char>(std::bit_cast<float>((uint32_t)result.value) > std::bit_cast<float>((uint32_t)right.value));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<char>(std::bit_cast<double>(result.value) > std::bit_cast<double>(right.value));
            break;
        default:
            result.value = (bool)(result.value > right.value);
            break;
        }
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator<=(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        switch (result.cType[0])
        {
        case CType::FLOAT:
            result.value = std::bit_cast<char>(std::bit_cast<float>((uint32_t)result.value) <= std::bit_cast<float>((uint32_t)right.value));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<char>(std::bit_cast<double>(result.value) <= std::bit_cast<double>(right.value));
            break;
        default:
            result.value = (bool)(result.value <= right.value);
            break;
        }
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator>=(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        switch (result.cType[0])
        {
        case CType::FLOAT:
            result.value = std::bit_cast<char>(std::bit_cast<float>((uint32_t)result.value) >= std::bit_cast<float>((uint32_t)right.value));
            break;
        case CType::DOUBLE:
            result.value = std::bit_cast<char>(std::bit_cast<double>(result.value) >= std::bit_cast<double>(right.value));
            break;
        default:
            result.value = (bool)(result.value >= right.value);
            break;
        }
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator&&(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        result.value = (bool)result.value && (bool)right.value;
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator||(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        result.value = (bool)result.value || (bool)right.value;
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator!() const
    {
        SymbolDescriptor result = *this;
        result.value = !((bool)result.value);
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator&(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        result.value &= right.value;
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator|(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        result.value &= right.value;
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator^(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        result.value &= right.value;
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator~() const
    {
        SymbolDescriptor result = *this;
        result.value = ~result.value;
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator<<(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        result.value <<= right.value;
        return result;
    }

    SymbolDescriptor SymbolDescriptor::operator>>(const SymbolDescriptor &right) const
    {
        SymbolDescriptor result = *this;
        result.value >>= right.value;
        return result;
    }

} // namespace CdbgExpr