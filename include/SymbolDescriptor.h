#ifndef _SYMBOL_DESCRIPTOR_H_
#define _SYMBOL_DESCRIPTOR_H_

#include <string>
#include <vector>

namespace CdbgExpr
{
    struct Scope
    {
        enum class Type
        {
            GLOBAL,
            FUNCTION,
            FILE,
            STRUCT,
            UNKNOWN
        };

        Type type = Type::UNKNOWN;
        std::string name;
    };

    enum class CType
    {
        VOID,
        INT,
        BOOL,
        CHAR,
        SHORT,
        LONG,
        FLOAT,
        DOUBLE,
        STRUCT,
        UNION,
        POINTER,
        ARRAY,
        UNKNOWN
    };

    class SymbolDescriptor;

    class DbgData
    {
    public:
        virtual SymbolDescriptor &getSymbol(const std::string &) = 0;
        virtual uint8_t getByte(size_t) = 0;
        virtual void setByte(size_t, uint8_t) = 0;
        virtual uint8_t CTypeSize(CType) = 0;
    };

    struct Member
    {
        SymbolDescriptor *symbol; // The symbol information for this member.
        int offset = 0;           // Member-specific offset.
    };

    class SymbolDescriptor
    {
    public:
        static DbgData* data;
        // Common fields.
        std::string name;   // Name of the symbol or type.
        Scope scope;        // The scope (global, file, function, or struct).
        uint64_t level = 0; // Additional context (for nested scopes).
        uint64_t block = 0;
        uint64_t value;
        bool hasAddress = false;
        uint64_t size = 0;

        // C type information.
        std::vector<CType> cType;
        std::vector<Member> members;

        static SymbolDescriptor strToNumber(const std::string &str);

        SymbolDescriptor dereference(int offset = 0) const;
        SymbolDescriptor getMember(const std::string &name) const;

        void setValue(uint64_t val);
        uint64_t getValue() const;

        std::string toString() const;

        SymbolDescriptor operator+(const SymbolDescriptor &right) const;
        SymbolDescriptor operator-(const SymbolDescriptor &right) const;
        SymbolDescriptor operator*(const SymbolDescriptor &right) const;
        SymbolDescriptor operator/(const SymbolDescriptor &right) const;
        SymbolDescriptor operator%(const SymbolDescriptor &right) const;
        SymbolDescriptor operator==(const SymbolDescriptor &right) const;
        SymbolDescriptor operator!=(const SymbolDescriptor &right) const;
        SymbolDescriptor operator<(const SymbolDescriptor &right) const;
        SymbolDescriptor operator>(const SymbolDescriptor &right) const;
        SymbolDescriptor operator<=(const SymbolDescriptor &right) const;
        SymbolDescriptor operator>=(const SymbolDescriptor &right) const;
        SymbolDescriptor operator&&(const SymbolDescriptor &right) const;
        SymbolDescriptor operator||(const SymbolDescriptor &right) const;
        SymbolDescriptor operator!() const;
        SymbolDescriptor operator&(const SymbolDescriptor &right) const;
        SymbolDescriptor operator|(const SymbolDescriptor &right) const;
        SymbolDescriptor operator^(const SymbolDescriptor &right) const;
        SymbolDescriptor operator~() const;
        SymbolDescriptor operator<<(const SymbolDescriptor &right) const;
        SymbolDescriptor operator>>(const SymbolDescriptor &right) const;
    };

} // namespace CdbgExpr

#endif // _SYMBOL_DESCRIPTOR_H_