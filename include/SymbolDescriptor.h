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

    class CType
    {
    public:
        enum class Type
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
            BITFIELD,
            UNKNOWN
        };
        Type type;

        char offset = 0;
        size_t size = 0;
        std::string name;

        CType() : type(Type::UNKNOWN) {}
        CType(CType::Type _type) : type(_type) {}
        CType(CType::Type _type, const std::string& structName) : type(_type), name(structName) {}

        bool operator==(const CType& right) const
        {
            if (right.type != type)
            {
                return false;
            }
            return true;
        }

        bool operator==(const CType::Type& right) const
        {
            if (right != type)
            {
                return false;
            }
            return true;
        }
        static std::vector<CType> parseCTypeVector(const std::string& typeStr, bool& isUnsigned);
    };

    class SymbolDescriptor;

    class DbgData
    {
    public:
        virtual SymbolDescriptor getSymbol(const std::string &) = 0;
        virtual uint8_t getByte(uint64_t) = 0;
        virtual void setByte(uint64_t, uint8_t) = 0;
        virtual uint8_t CTypeSize(CType) = 0;
        virtual uint64_t getStackPointer() = 0;
        virtual uint8_t getRegContent(uint8_t regNum) = 0;
        virtual void setRegContent(uint8_t regNum, uint8_t val) = 0;
        uint64_t invalidAddress = 0; // non-valid memory address (eg. nullptr/0)
    };

    class Member;

    class SymbolDescriptor
    {
    public:
        static DbgData* data;
        static bool assignmentAllowed;

        std::string name;
        uint64_t value;
        bool hasAddress = false;
        uint64_t size = 0;
        bool isSigned = false;
        std::unordered_map<std::string, Member> members;

        bool stack = false;
        int stackOffs = 0;

        std::vector<uint8_t> regs;

        // C type information.
        std::vector<CType> cType;

        SymbolDescriptor() = default;
        SymbolDescriptor(const char* s);
        SymbolDescriptor(const std::string& s);
        SymbolDescriptor(double d);
        SymbolDescriptor(int64_t i);
        SymbolDescriptor(uint64_t u);

        std::variant<uint64_t, int64_t, double, float> getRealValue(const std::vector<uint64_t>& offset = {}) const;

        static uint64_t value_to_uint64_n(uint64_t val, CType type);
        static int64_t value_to_int64_n(uint64_t val, CType type);
        static double value_to_double_b(uint64_t val);
        static double value_to_double_n(uint64_t val, CType type, bool isSigned = false);
        static float value_to_float_b(uint64_t val);
        static float value_to_float_n(uint64_t val, CType type, bool isSigned = false);

        static size_t getItemSize(const std::vector<CType>& cType, uint8_t level = 0);
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

        void setAddr(uint64_t addr);

        void setValue(uint64_t val);
        uint64_t getValue() const;
        void setValueAt(uint64_t addr, uint64_t val, uint8_t level = 0);
        uint64_t getValueAt(uint64_t addr, uint8_t level = 0) const;

        std::string typeOf() const;
        std::string toString() const;

        SymbolDescriptor assign(const SymbolDescriptor &right);

        SymbolDescriptor getConstLiteral(const std::vector<uint64_t>& offset) const;

        float toFloat(const std::vector<uint64_t>& offset = {}) const;
        double toDouble(const std::vector<uint64_t>& offset = {}) const;
        uint64_t toUnsigned(const std::vector<uint64_t>& offset = {}) const;
        int64_t toSigned(const std::vector<uint64_t>& offset = {}) const;

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

    class Member
    {
    public:
        SymbolDescriptor symbol;  // The symbol information for this member.
        int offset = 0;           // Member-specific offset.
    };

} // namespace CdbgExpr

#endif // _SYMBOL_DESCRIPTOR_H_
