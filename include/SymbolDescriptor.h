#ifndef _SYMBOL_DESCRIPTOR_H_
#define _SYMBOL_DESCRIPTOR_H_

#include <string>
#include <vector>
#include <cstdint>
#include <variant>
#include <unordered_map>

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
        LONGLONG,
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
        virtual SymbolDescriptor getSymbol(const std::string &) = 0;
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

        std::string name;
        std::variant<uint64_t, int64_t, double, float> value;
        bool hasAddress = false;
        uint64_t size = 0;
        bool isSigned = false;
        std::unordered_map<std::string, Member> members;

        // C type information.
        std::vector<CType> cType;

        SymbolDescriptor() = default;
        SymbolDescriptor(const char* s);
        SymbolDescriptor(const std::string& s);
        SymbolDescriptor(double d);
        SymbolDescriptor(int64_t i);
        SymbolDescriptor(uint64_t u);

        static uint64_t value_to_uint64_b(const std::variant<uint64_t, int64_t, double, float> &v);
        static uint64_t value_to_uint64_n(const std::variant<uint64_t, int64_t, double, float> &v);
        static int64_t value_to_int64_b(const std::variant<uint64_t, int64_t, double, float> &v);
        static int64_t value_to_int64_n(const std::variant<uint64_t, int64_t, double, float> &v);
        static double value_to_double_b(const std::variant<uint64_t, int64_t, double, float> &v);
        static double value_to_double_n(const std::variant<uint64_t, int64_t, double, float> &v);
        static float value_to_float_b(const std::variant<uint64_t, int64_t, double, float> &v);
        static float value_to_float_n(const std::variant<uint64_t, int64_t, double, float> &v);

        static CType promoteType(const CType &left, const CType &right);

        void fromString(const std::string &str);
        void fromDouble(const double &val);
        void fromInt(const int64_t &val);
        void fromUint(const uint64_t &val);

        
        friend std::ostream& operator<<(std::ostream& os, const SymbolDescriptor& obj)
        {
            os << obj.toString();
            return os;
        }

        SymbolDescriptor dereference(int offset = 0) const;
        SymbolDescriptor getMember(const std::string &name) const;
        SymbolDescriptor addressOf() const;

        void setValue(const std::variant<uint64_t, int64_t, double, float>& val);
        std::variant<uint64_t, int64_t, double, float> getValue() const;

        std::string toString() const;

        SymbolDescriptor assign(const SymbolDescriptor &right);

        float toFloat() const;
        double toDouble() const;
        uint64_t toUnsigned() const;
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
