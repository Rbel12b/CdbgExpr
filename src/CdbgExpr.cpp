#include "CdbgExpr.h"
#include <cstdint>
#include <iostream>
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
        int idx = index.toUnsigned();
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

    DbgData *ASTNode::data = nullptr;

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
            return std::make_unique<LiteralNode>(SymbolDescriptor(token.value));
        }
        else if (token.type == TokenType::SYMBOL)
        {
            return std::make_unique<SymbolNode>(token.value);
        }
        else if (token.type == TokenType::PARENTHESIS && token.value == "(")
        {
            auto expr = parseExpression(1);
            if (index >= tokens.size() || (tokens[index].type != TokenType::PARENTHESIS && tokens[index].value != ")"))
            {
                if (index >= tokens.size())
                {
                    throw std::runtime_error("Expected closing parenthesis, index: " + std::to_string(index));
                }
                throw std::runtime_error("Expected closing parenthesis: " + tokens[index].value + ", index: " + std::to_string(index));
            }
            ++index;
            return expr;
        }
        throw std::runtime_error("Unexpected token in primary expression: " + token.value + ", index: " + std::to_string(index - 1));
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
} // namespace CdbgExpr