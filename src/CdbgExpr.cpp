#include "CdbgExpr.h"
#include "SymbolDescriptor.h"
#include <sstream>
#include <bit>
#include <cstdint>
#include <iostream>
#include <stack>
#include <vector>
#include <unordered_map>

namespace CdbgExpr
{
    Expression::Expression(const std::string &expr, DbgData *dbgData)
        : expr_(expr), pos_(0), dbgData(dbgData)
    {
        SymbolDescriptor::data = dbgData;
        ASTNode::data = dbgData;
    }

    SymbolDescriptor Expression::eval(bool assignmentAllowed)
    {
        tokenize(expr_);
        pos_ = 0;
        SymbolDescriptor::assignmentAllowed = assignmentAllowed;
        ExpressionParser expParser(tokens, dbgData);
        return expParser.parse()->evaluate(); // Evaluate the expression
    }

    Expression::~Expression() {}

    void Expression::tokenize(const std::string &expr)
    {
        std::string token;
        Token currentToken;
        uint64_t i = 0;
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
            if (c == '+' || c == '-' || c == '*' || c == '&' || c == '=')
            {
                if (tokens.empty() || (tokens.back().type != TokenType::SYMBOL && tokens.back().type != TokenType::NUMBER))
                {
                    tokens.push_back({TokenType::UNARY_OPERATOR, std::string(1, c)});
                }
                else
                {
                    tokens.push_back({TokenType::OPERATOR, std::string(1, c)});
                }
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

    int getPrecedence(const Token &token)
    {
        const std::unordered_map<std::string, int> precedence = {
            {"[]", 17}, {"++", 15}, {"--", 15}, {"*", 14}, {"/", 14}, {"%", 14},
            {"+", 13}, {"-", 13}, {"<<", 12}, {">>", 12}, {"<", 11}, {">", 11}, {"<=", 11}, {">=", 11},
            {"==", 10}, {"!=", 10}, {"&", 9}, {"^", 8}, {"|", 7}, {"&&", 6}, {"||", 5}, {"?", 4}, {":", 4},
            {"=", 3}, {"+=", 3}, {"-=", 3}, {"*=", 3}, {"/=", 3}, {"%=", 3}, {"<<=", 3}, {">>=", 3},
            {"&=", 3}, {"^=", 3}, {"|=", 3}, {",", 2},
        };

        if (token.type == TokenType::PARENTHESIS)
        {
            return 0;
        }
        else if (token.type == TokenType::STRUCT_ACCESS)
        {    
            return 18;
        }
        else if (token.type == TokenType::UNARY_OPERATOR)
        {    
            return 16;
        }
        else if (precedence.contains(token.value))
        {
            return precedence.at(token.value);
        }
        return 0;
    }

    SymbolDescriptor evalUnaryOperator(const SymbolDescriptor &operand, const std::string &op)
    {
        if (op == "-")
        {
            return -operand;
        }
        else if (op == "+")
        {
            return operand;
        }
        else if (op == "*")
        {
            return operand.dereference();
        }
        else if (op == "&")
        {
            return operand.addressOf();
        }
        throw std::runtime_error("Unsupported unary operator: " + op);
    }

    SymbolDescriptor evalBinaryOperator(SymbolDescriptor &left, const SymbolDescriptor &right, const std::string &op)
    {
        if (op == "=")
        {
            return left.assign(right);
        }
        else if (op == "+=")
        {
            return left.assign(left + right);
        }
        else if (op == "-=")
        {
            return left.assign(left - right);
        }
        else if (op == "*=")
        {
            return left.assign(left * right);
        }
        else if (op == "/=")
        {
            return left.assign(left / right);
        }
        else if (op == "%=")
        {
            return left.assign(left % right);
        }
        else if (op == "&=")
        {
            return left.assign(left & right);
        }
        else if (op == "|=")
        {
            return left.assign(left | right);
        }
        else if (op == "^=")
        {
            return left.assign(left ^ right);
        }
        else if (op == "<<=")
        {
            return left.assign(left << right);
        }
        else if (op == ">>=")
        {
            return left.assign(left >> right);
        }
        return evalArithmeticOperator(left, right, op);
    }

    SymbolDescriptor evalArithmeticOperator(const SymbolDescriptor &left, const SymbolDescriptor &right, const std::string &op)
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
            return evalArrayAccess(left, right);
        }
        else if (op == "." || op == "->")
        {
            return evalMemberAccess(left, right.name, op == "->");
        }
        throw std::runtime_error("Unsupported binary operator: " + op);
    }

    SymbolDescriptor evalArrayAccess(const SymbolDescriptor &array, const SymbolDescriptor &index)
    {
        if (array.cType[0] != CType::POINTER && array.cType[0] != CType::ARRAY)
        {
            throw std::runtime_error("Cannot index a non-array type");
        }
        int idx = index.getValue();
        return array.dereference(idx);
    }

    SymbolDescriptor evalMemberAccess(const SymbolDescriptor &structOrPointer, const std::string &member, bool isPointerAccess)
    {
        if (isPointerAccess && structOrPointer.cType[0] != CType::POINTER)
        {
            throw std::runtime_error("Expected a pointer for '->' operator");
        }
        const SymbolDescriptor &baseStruct = isPointerAccess ? structOrPointer.dereference() : structOrPointer;
        return baseStruct.getMember(member);
    }

    BinaryOpNode::BinaryOpNode(std::string op, std::unique_ptr<ASTNode> lhs, std::unique_ptr<ASTNode> rhs)
        : op(std::move(op)), left(std::move(lhs)), right(std::move(rhs)) {}

    SymbolDescriptor BinaryOpNode::evaluate()
    {
        SymbolDescriptor lhsVal = left->evaluate();
        SymbolDescriptor rhsVal = right->evaluate();

        return evalBinaryOperator(lhsVal, rhsVal, op);
    }

    UnaryOpNode::UnaryOpNode(std::string op, std::unique_ptr<ASTNode> operand)
        : op(std::move(op)), operand(std::move(operand)) {}

    SymbolDescriptor UnaryOpNode::evaluate()
    {
        SymbolDescriptor value = operand->evaluate();

        return evalUnaryOperator(value, op);
    }

    LiteralNode::LiteralNode(SymbolDescriptor val) : value(std::move(val)) {}

    SymbolDescriptor LiteralNode::evaluate()
    {
        return value;
    }

    SymbolNode::SymbolNode(std::string name)
        : name(std::move(name)) {}

    SymbolDescriptor SymbolNode::evaluate()
    {
        return data->getSymbol(name);
    }

    std::unique_ptr<ASTNode> ExpressionParser::parsePrimary()
    {
        if (index >= tokens.size())
            throw std::runtime_error("Unexpected end of input, index: " + std::to_string(index));

        Token token = tokens[index++];
        if (token.type == TokenType::NUMBER)
        {
            return std::make_unique<LiteralNode>(SymbolDescriptor::strToNumber(token.value));
        }
        else if (token.type == TokenType::SYMBOL)
        {
            return std::make_unique<SymbolNode>(token.value);
        }
        else if (token.type == TokenType::PARENTHESIS && token.value == "(")
        {
            auto expr = parseExpression(1);
            if (tokens[index].type != TokenType::PARENTHESIS && tokens[index].value != ")")
            {
                throw std::runtime_error("Expected closing parenthesis: " + tokens[index].value + ", index: " + std::to_string(index));
            }
            ++index;
            return expr;
        }
        throw std::runtime_error("Unexpected token in primary expression: " + tokens[index].value + ", index: " + std::to_string(index));
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseExpression(int minPrecedence)
    {
        if (minPrecedence == 0)
        {
            minPrecedence = 1;
        }
        std::unique_ptr<ASTNode> lhs;

        if (index >= tokens.size())
            throw std::runtime_error("Unexpected end of input");

        Token token = tokens[index];
        if (token.type == TokenType::UNARY_OPERATOR)
        {
            index++; // Consume the operator
            auto operand = parseExpression(getPrecedence(token) + 1);
            lhs = std::make_unique<UnaryOpNode>(token.value, std::move(operand));
        }
        else
        {
            lhs = parsePrimary();
        }

        // Handle binary operators
        while (index < tokens.size() && getPrecedence(tokens[index]) >= minPrecedence)
        {
            Token opToken = tokens[index++];
            int precedence = getPrecedence(opToken);
            auto rhs = parseExpression(precedence + 1);
            lhs = std::make_unique<BinaryOpNode>(opToken.value, std::move(lhs), std::move(rhs));
        }

        return lhs;
    }

    ExpressionParser::ExpressionParser(std::vector<Token> tokens, DbgData *debuggerData)
        : tokens(std::move(tokens)), index(0), debuggerData(debuggerData) {}

    std::unique_ptr<ASTNode> ExpressionParser::parse()
    {
        return parseExpression(1);
    }

    // Symbol handling functions.

    DbgData *ASTNode::data = nullptr;
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