#ifndef _CDBG_EXPR_H_
#define _CDBG_EXPR_H_

#include <string>
#include <vector>
#include <stdexcept>
#include <cctype>
#include <functional>
#include <memory>
#include "SymbolDescriptor.h"

namespace CdbgExpr
{
    enum class TokenType
    {
        SYMBOL,
        NUMBER,
        OPERATOR,
        UNARY_OPERATOR,
        PARENTHESIS,
        ARRAY_ACCESS,
        STRUCT_ACCESS,
        ASSIGNMENT,
        FUNCTION_CALL,
    };

    struct Token
    {
        TokenType type;
        std::string value;
    };

    class Expression
    {
    public:
        Expression(const std::string &expr, DbgData *dbgData);
        ~Expression();

        SymbolDescriptor eval(bool assignmentAllowed);

    private:
        std::string expr_;
        uint64_t pos_;
        DbgData *dbgData;
        std::vector<Token> tokens;

        void tokenize(const std::string &expr);
    };

    SymbolDescriptor evalUnaryOperator(const SymbolDescriptor &operand, const std::string &op);
    SymbolDescriptor evalBinaryOperator(SymbolDescriptor &left, const SymbolDescriptor &right, const std::string &op);
    SymbolDescriptor evalArithmeticOperator(const SymbolDescriptor &left, const SymbolDescriptor &right, const std::string &op);
    SymbolDescriptor evalArrayAccess(const SymbolDescriptor &array, const SymbolDescriptor &index);
    SymbolDescriptor evalMemberAccess(const SymbolDescriptor &structOrPointer, const std::string &member, bool isPointerAccess);
    int getPrecedence(const Token &token);

    class ASTNode
    {
    public:
        virtual ~ASTNode() = default;
        virtual SymbolDescriptor evaluate() = 0;
        static DbgData *data;
    };

    class BinaryOpNode : public ASTNode
    {
    public:
        std::string op;
        std::unique_ptr<ASTNode> left, right;

        BinaryOpNode(std::string op, std::unique_ptr<ASTNode> lhs, std::unique_ptr<ASTNode> rhs);

        SymbolDescriptor evaluate() override;
    };

    class UnaryOpNode : public ASTNode
    {
    public:
        std::string op;
        std::unique_ptr<ASTNode> operand;

        UnaryOpNode(std::string op, std::unique_ptr<ASTNode> operand);

        SymbolDescriptor evaluate() override;
    };

    class LiteralNode : public ASTNode
    {
    public:
        SymbolDescriptor value;

        explicit LiteralNode(SymbolDescriptor val);

        SymbolDescriptor evaluate() override;
    };

    class SymbolNode : public ASTNode
    {
    public:
        std::string name;

        SymbolNode(std::string name);

        SymbolDescriptor evaluate() override;
    };


    class ExpressionParser
    {
        std::vector<Token> tokens;
        size_t index;
        DbgData *debuggerData;

        std::unique_ptr<ASTNode> parsePrimary();

        std::unique_ptr<ASTNode> parseExpression(int minPrecedence);

    public:
        ExpressionParser(std::vector<Token> tokens, DbgData *debuggerData);

        std::unique_ptr<ASTNode> parse();
    };

} // namespace CdbgExpr

#endif // _CDBG_EXPR_H_
