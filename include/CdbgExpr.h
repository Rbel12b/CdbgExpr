#ifndef _CDBG_EXPR_H_
#define _CDBG_EXPR_H_

#include <string>
#include <vector>
#include <stdexcept>
#include <cctype>
#include <functional>
#include "SymbolDescriptor.h"

namespace CdbgExpr
{
    class Expression
    {
    public:
        enum class TokenType
        {
            SYMBOL,
            NUMBER,
            OPERATOR,
            PARENTHESIS,
            ARRAY_ACCESS,
            STRUCT_ACCESS,
            ASSIGNMENT
        };


        struct Token {
            TokenType type;
            std::string value;
            std::vector<Token> children;
        };

        Expression(const std::string &expr, DbgData *dbgData)
            : expr_(expr), pos_(0), dbgData(dbgData), assignmentAllowed_(false)
        {
        }
        ~Expression();

        SymbolDescriptor eval(bool assignmentAllowed);

    private:
        std::string expr_;
        uint64_t pos_;
        DbgData *dbgData;
        bool assignmentAllowed_;
        std::vector<Token> tokens;

        void tokenize(const std::string &expr);
        void tokenizeNested(const std::string& expr, Token& token);
        SymbolDescriptor parseExpression();
        SymbolDescriptor parseExpression(std::vector<Token> &tokens);
        SymbolDescriptor parseTerm(std::vector<Token> &tokens);
        SymbolDescriptor parseFactor(std::vector<Token> &tokens);
        SymbolDescriptor parseAssignment();
        SymbolDescriptor parseArrayAccess(SymbolDescriptor symbol, std::vector<Token> &tokens);
        SymbolDescriptor parseStructAccess(SymbolDescriptor symbol, std::vector<Token> &tokens);
        SymbolDescriptor parseUnaryOperator(SymbolDescriptor symbol);
        SymbolDescriptor parseBinaryOperator(SymbolDescriptor left, SymbolDescriptor right);

        // void skipWhitespace()
        // {
        //     while (pos_ < expr_.size() && std::isspace(expr_[pos_]))
        //     {
        //         ++pos_;
        //     }
        // }

        // SymbolDescriptor parseAssignment()
        // {
        //     skipWhitespace();
        //     uint64_t startPos = pos_;

        //     if (pos_ < expr_.size() && (std::isalpha(expr_[pos_]) || expr_[pos_] == '_'))
        //     {
        //         std::string id = parseIdentifier();
        //         skipWhitespace();
        //         if (pos_ < expr_.size() && expr_[pos_] == '=')
        //         {
        //             if (!assignmentAllowed_)
        //             {
        //                 throw std::runtime_error("Assignment not allowed.");
        //             }

        //             pos_++;
        //             SymbolDescriptor rhs = parseAssignment();
        //             SymbolDescriptor &sym = dbgData_->getSymbol(id);
        //             sym.value = rhs.value;
        //             return rhs;
        //         }
        //         else
        //         {
        //             pos_ = startPos;
        //         }
        //     }
        //     return parseMemberAccess();
        // }

        // SymbolDescriptor parseMemberAccess()
        // {
        //     SymbolDescriptor value = parseAddSub();
        //     skipWhitespace();
        //     while (pos_ < expr_.size() && expr_[pos_] == '.')
        //     {
        //         pos_++;
        //         std::string memberName = parseIdentifier();
        //         if (value.cType.empty() || value.cType[0] != CType::STRUCT)
        //         {
        //             throw std::runtime_error("Attempted member access on non-struct");
        //         }
        //         bool found = false;
        //         for (const auto &member : value.members)
        //         {
        //             if (member.symbol->name == memberName)
        //             {
        //                 value = *member.symbol;
        //                 found = true;
        //                 break;
        //             }
        //         }
        //         if (!found)
        //         {
        //             throw std::runtime_error("Struct member not found: " + memberName);
        //         }
        //         skipWhitespace();
        //     }
        //     return value;
        // }

        // SymbolDescriptor parseAddSub()
        // {
        //     SymbolDescriptor value = parseMulDiv();
        //     skipWhitespace();
        //     while (pos_ < expr_.size() && (expr_[pos_] == '+' || expr_[pos_] == '-'))
        //     {
        //         char op = expr_[pos_++];
        //         SymbolDescriptor rhs = parseMulDiv();
        //         value.value = (op == '+') ? (intptr_t)value.value + (intptr_t)rhs.value : (intptr_t)value.value - (intptr_t)rhs.value;
        //         skipWhitespace();
        //     }
        //     return value;
        // }

        // SymbolDescriptor parseMulDiv()
        // {
        //     SymbolDescriptor value = parseUnary();
        //     skipWhitespace();
        //     while (pos_ < expr_.size() && (expr_[pos_] == '*' || expr_[pos_] == '/'))
        //     {
        //         char op = expr_[pos_++];
        //         SymbolDescriptor rhs = parseUnary();
        //         if (op == '/' && (intptr_t)rhs.value == 0)
        //         {
        //             throw std::runtime_error("Division by zero");
        //         }
        //         value.value = ((op == '*') ? (intptr_t)value.value * (intptr_t)rhs.value : (intptr_t)value.value / (intptr_t)rhs.value);
        //         skipWhitespace();
        //     }
        //     return value;
        // }

        // SymbolDescriptor parseUnary()
        // {
        //     skipWhitespace();
        //     if (pos_ < expr_.size() && expr_[pos_] == '*')
        //     {
        //         pos_++;
        //         SymbolDescriptor ptr = parseUnary();
        //         if (ptr.cType.empty() || ptr.cType[0] != CType::POINTER)
        //         {
        //             throw std::runtime_error("Cannot dereference non-pointer");
        //         }
        //         return dbgData_->dereference(ptr);
        //     }
        //     else if (pos_ < expr_.size() && expr_[pos_] == '-')
        //     {
        //         pos_++;
        //         SymbolDescriptor value = parseUnary();
        //         value.value = ((intptr_t)value.value * -1);
        //         return value;
        //     }
        //     return parsePrimary();
        // }

        // SymbolDescriptor parsePrimary()
        // {
        //     skipWhitespace();

        //     if (pos_ < expr_.size() && expr_[pos_] == '(')
        //     {
        //         pos_++;
        //         SymbolDescriptor value = parseAssignment();
        //         skipWhitespace();
        //         if (pos_ >= expr_.size() || expr_[pos_] != ')')
        //             throw std::runtime_error("Expected closing parenthesis");
        //         pos_++;
        //         return value;
        //     }
        //     else if (pos_ < expr_.size() && std::isdigit(expr_[pos_]))
        //     {
        //         SymbolDescriptor value;
        //         value.value = (intptr_t)parseNumber();
        //         value.cType = {CType::INT};
        //         return value;
        //     }
        //     else if (pos_ < expr_.size() && (std::isalpha(expr_[pos_]) || expr_[pos_] == '_'))
        //     {
        //         std::string id = parseIdentifier();
        //         SymbolDescriptor sym = dbgData_->getSymbol(id);

        //         while (pos_ < expr_.size() && expr_[pos_] == '[')
        //         {
        //             if (sym.cType.empty() || (sym.cType[0] != CType::POINTER && sym.cType[0] != CType::ARRAY))
        //             {
        //                 throw std::runtime_error("Invalid indexing on non-array type");
        //             }
        //             pos_++; // Consume '['

        //             uint64_t index = parseAssignment().value;

        //             skipWhitespace();

        //             if (pos_ >= expr_.size() || expr_[pos_] != ']')
        //             {
        //                 throw std::runtime_error("Expected closing bracket in array index");
        //             }

        //             pos_++; // Consume ']'

        //             if (sym.cType[0] == CType::ARRAY && index >= sym.size)
        //             {
        //                 throw std::runtime_error("Array index out of bounds");
        //             }

        //             if (sym.cType.size() < 2)
        //             {
        //                 throw std::runtime_error("Array must have a type");
        //             }

        //             uint64_t elementSize = dbgData_->CTypeSize(sym.cType[1]);
        //             sym = dbgData_->dereference(sym, index * elementSize);
        //         }

        //         return sym;
        //     }

        //     throw std::runtime_error("Unexpected token in expression");
        // }

        // int parseNumber()
        // {
        //     skipWhitespace();
        //     int value = 0;
        //     while (pos_ < expr_.size() && std::isdigit(expr_[pos_]))
        //     {
        //         value = value * 10 + (expr_[pos_++] - '0');
        //     }
        //     return value;
        // }

        // std::string parseIdentifier()
        // {
        //     skipWhitespace();
        //     std::string id;
        //     while (pos_ < expr_.size() && (std::isalnum(expr_[pos_]) || expr_[pos_] == '_'))
        //     {
        //         id.push_back(expr_[pos_++]);
        //     }
        //     return id;
        // }
    };
} // namespace CdbgExpr

#endif // _CDBG_EXPR_H_
