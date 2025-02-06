#ifndef _SYMBOL_DESCRIPTOR_H_
#define _SYMBOL_DESCRIPTOR_H_

#include <string>
#include <vector>
#include <cstdint>

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
        virtual uint8_t getByte(uint64_t) = 0;
        virtual void setByte(uint64_t, uint8_t) = 0;
        virtual uint8_t CTypeSize(CType) = 0;
        uint64_t invalidAddress = 0;
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
        static bool assignmentAllowed;
        // Common fields.
        std::string name;   // Name of the symbol or type.
        uint64_t value;
        bool hasAddress = false;
        uint64_t size = 0;
        bool isSigned = false;

        // C type information.
        std::vector<CType> cType;
        std::vector<Member> members;

        static SymbolDescriptor strToNumber(const std::string &str);

        SymbolDescriptor dereference(int offset = 0) const;
        SymbolDescriptor getMember(const std::string &name) const;
        SymbolDescriptor addressOf() const;

        void setValue(uint64_t val);
        uint64_t getValue() const;

        std::string toString() const;

        SymbolDescriptor assign(const SymbolDescriptor &right);

        float toFloat() const;
        double toDouble() const;
        int64_t toSigned() const;

        template <typename Op> SymbolDescriptor applyArithmetic(const SymbolDescriptor &right, Op op) const;
        template <typename Op> SymbolDescriptor applyComparison(const SymbolDescriptor &right, Op op) const;
        template <typename Op> SymbolDescriptor applyLogical(const SymbolDescriptor &right, Op op) const;
        template <typename Op> SymbolDescriptor applyBitwise(const SymbolDescriptor &right, Op op) const;
        
        SymbolDescriptor operator+(const SymbolDescriptor &right) const;
        SymbolDescriptor operator-(const SymbolDescriptor &right) const;
        SymbolDescriptor operator-() const;
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
