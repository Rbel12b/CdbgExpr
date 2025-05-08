#include "CdbgExpr.h"
#include <cstdint>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <set>

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
        std::vector<std::string> rawTokens;
        std::string token;
        uint64_t i = 0;

        static const std::set<std::string> multiCharOperators = {
            "==", "!=", "<=", ">=", "&&", "||", "->", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<", ">>"
        };

        // First Pass: Raw token extraction
        while (i < expr.size())
        {
            char c = expr[i];

            if (std::isspace(c))
            {
                i++;
                continue;
            }

            // --- STRING LITERALS ---
            if (c == '"' || c == '\'')
            {
                char quote = c;
                token.clear();
                token += quote;
                i++;
                while (i < expr.size())
                {
                    char ch = expr[i];
                    token += ch;
                    i++;
                    if (ch == '\\' && i < expr.size())  // Escape character
                    {
                        token += expr[i];
                        i++;
                        continue;
                    }
                    if (ch == quote)
                        break;
                }
                rawTokens.push_back(token);
                continue;
            }

            // --- FLOATING POINT OR INTEGER NUMBERS ---
            if (std::isdigit(c) || (c == '.' && i + 1 < expr.size() && std::isdigit(expr[i + 1])) || 
                (c == '0' && i + 1 < expr.size() && (expr[i + 1] == 'x' || expr[i + 1] == 'X' || expr[i + 1] == 'b' || expr[i + 1] == 'B')))
            {
                token.clear();
                size_t start = i;

                if (expr[i] == '0' && i + 1 < expr.size() && (expr[i + 1] == 'x' || expr[i + 1] == 'X'))
                {
                    // Hex literal: 0x...
                    token += expr[i++];
                    token += expr[i++];
                    while (i < expr.size() && std::isxdigit(expr[i]))
                    {
                        token += expr[i++];
                    }
                }
                else if (expr[i] == '0' && i + 1 < expr.size() && (expr[i + 1] == 'b' || expr[i + 1] == 'B'))
                {
                    // Binary literal: 0b...
                    token += expr[i++];
                    token += expr[i++];
                    while (i < expr.size() && (expr[i] == '0' || expr[i] == '1'))
                    {
                        token += expr[i++];
                    }
                }
                else
                {
                    // Decimal or floating point
                    bool hasDot = false;
                    bool hasExp = false;
                    while (i < expr.size())
                    {
                        char ch = expr[i];
                        if (std::isdigit(ch))
                        {
                            token += ch;
                            i++;
                        }
                        else if (ch == '.' && !hasDot)
                        {
                            hasDot = true;
                            token += ch;
                            i++;
                        }
                        else if ((ch == 'e' || ch == 'E') && !hasExp)
                        {
                            hasExp = true;
                            token += ch;
                            i++;
                            if (i < expr.size() && (expr[i] == '+' || expr[i] == '-'))
                            {
                                token += expr[i++];
                            }
                        }
                        else
                        {
                            break;
                        }
                    }

                    // Optional float suffix (f/F, l/L)
                    if (i < expr.size() && (expr[i] == 'f' || expr[i] == 'F' || expr[i] == 'l' || expr[i] == 'L'))
                    {
                        token += expr[i++];
                    }
                }

                // Optional integer suffix (u/U, l/L, ul/UL, etc.)
                while (i < expr.size() && (expr[i] == 'u' || expr[i] == 'U' || expr[i] == 'l' || expr[i] == 'L'))
                {
                    token += expr[i++];
                }

                rawTokens.push_back(token);
                continue;
            }

            // --- SYMBOLS (identifiers) ---
            if (std::isalpha(c) || c == '_')
            {
                token.clear();
                while (i < expr.size() && (std::isalnum(expr[i]) || expr[i] == '_'))
                {
                    token += expr[i];
                    i++;
                }
                rawTokens.push_back(token);
                continue;
            }

            // --- MULTI-CHARACTER OPERATORS ---
            bool matched = false;
            for (int len = 3; len >= 2; --len)
            {
                if (i + len <= expr.size())
                {
                    std::string op = expr.substr(i, len);
                    if (multiCharOperators.count(op))
                    {
                        rawTokens.push_back(op);
                        i += len;
                        matched = true;
                        break;
                    }
                }
            }
            if (matched) continue;

            // --- SINGLE CHARACTER TOKEN ---
            rawTokens.push_back(std::string(1, expr[i]));
            i++;
        }

        // Second Pass: Type assignment
        for (size_t j = 0; j < rawTokens.size(); ++j)
        {
            const std::string &tok = rawTokens[j];
            TokenType type;

            if (tok == "(" || tok == ")")
            {
                type = TokenType::PARENTHESIS;
            }
            else if (tok == "[" || tok == "]")
            {
                type = TokenType::ARRAY_ACCESS;
            }
            else if (tok == "." || tok == "->")
            {
                type = TokenType::STRUCT_ACCESS;
            }
            else if ((tok.front() == '"' && tok.back() == '"') || (tok.front() == '\'' && tok.back() == '\''))
            {
                type = TokenType::STRING_LITERAL;
            }
            else if (std::isdigit(tok[0]) || tok[0] == '.')
            {
                type = TokenType::NUMBER;
            }
            else if (std::isalpha(tok[0]) || tok[0] == '_')
            {
                type = TokenType::SYMBOL;
            }
            else if (tok == "+" || tok == "-" || tok == "*" || tok == "&" || tok == "=")
            {
                if (tokens.empty() || (tokens.back().type != TokenType::SYMBOL &&
                                    tokens.back().type != TokenType::NUMBER &&
                                    tokens.back().type != TokenType::PARENTHESIS &&
                                    tokens.back().value != ")"))
                    type = TokenType::UNARY_OPERATOR;
                else
                    type = TokenType::OPERATOR;
            }
            else
            {
                type = TokenType::OPERATOR;
            }

            tokens.push_back({type, tok});
        }
    }

    int getPrecedence(const Token &token)
    {
        static const std::unordered_map<std::string, int> precedence = {
            {"[]", 18}, {".", 18}, {"->", 18},                    // member access
            {"++", 17}, {"--", 17},                               // postfix
            {"*", 15}, {"/", 15}, {"%", 15},                      // multiplicative
            {"+", 14}, {"-", 14},                                 // additive
            {"<<", 13}, {">>", 13},                               // shift
            {"<", 12}, {"<=", 12}, {">", 12}, {">=", 12},         // relational
            {"==", 11}, {"!=", 11},                               // equality
            {"&", 10},
            {"^", 9},
            {"|", 8},
            {"&&", 7},
            {"||", 6},
            {"?", 5}, {":", 5},
            {"=", 4}, {"+=", 4}, {"-=", 4}, {"*=", 4}, {"/=", 4},
            {"%=", 4}, {"<<=", 4}, {">>=", 4}, {"&=", 4}, {"^=", 4}, {"|=", 4},
            {",", 3}
        };

        if (token.type == TokenType::PARENTHESIS)
        {
            return -1; // used structurally, not as operators
        }
        else if (token.type == TokenType::STRUCT_ACCESS)
        {
            return 18;
        }
        else if (token.type == TokenType::UNARY_OPERATOR)
        {
            return 16; // for unary + - * & ! ~
        }
        else if (precedence.contains(token.value))
        {
            return precedence.at(token.value);
        }
        return 0; // unknown or non-operator
    }

    bool isRightAssociative(const Token &token)
    {
        static const std::set<std::string> rightAssoc = {
            "=", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|=", "?", ":"
        };
        return rightAssoc.count(token.value) > 0 || token.type == TokenType::UNARY_OPERATOR;
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
        if (array.cType[0] != CType::Type::POINTER && array.cType[0] != CType::Type::ARRAY)
        {
            throw std::runtime_error("Cannot index a non-array type");
        }
        int idx = index.toUnsigned();
        return array.dereference(idx);
    }

    SymbolDescriptor evalMemberAccess(const SymbolDescriptor &structOrPointer, const std::string &member, bool isPointerAccess)
    {
        if (isPointerAccess && structOrPointer.cType[0] != CType::Type::POINTER)
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
            bool rightAssoc = isRightAssociative(opToken);

            auto rhs = parseExpression(precedence + (rightAssoc ? 0 : 1));
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