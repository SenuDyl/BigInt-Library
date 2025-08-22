#include <iostream>
#include <vector>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <random>
using namespace std;

struct BigInt
{
    vector<uint32_t> chunks;

    BigInt() {}
    BigInt(uint64_t value)
    {
        while (value > 0)
        {
            chunks.push_back((uint32_t)(value & 0xFFFFFFFFu)); 
            value >>= 32;                                      
        }
        normalize();
    }

    void normalize()
    {
        while (!chunks.empty() && chunks.back() == 0)
        {
            chunks.pop_back(); // Remove most-significant zero chunks
        }
    }

    bool is_zero() const
    {
        return chunks.empty();
    }

    static BigInt from_hex(const string &hex)
    {
        BigInt result;
        string s = hex;
        if (s.size() >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
            s = s.substr(2);

        reverse(s.begin(), s.end()); // process from least significant digit
        uint64_t current = 0;
        int bits = 0;
        for (char c : s)
        {
            int val;
            if (c >= '0' && c <= '9')
                val = c - '0';
            else if (c >= 'a' && c <= 'f')
                val = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                val = c - 'A' + 10;
            else
                continue;

            current |= (uint64_t)val << bits;
            bits += 4;
            if (bits >= 32)
            {
                result.chunks.push_back((uint32_t)(current & 0xFFFFFFFFu));
                current >>= 32;
                bits -= 32;
            }
        }
        if (bits > 0)
            result.chunks.push_back((uint32_t)current);
        result.normalize();
        return result;
    }

    static string to_hex(const BigInt &x)
    {
        if (x.is_zero())
            return "0x0"; // prepend 0x for zero
        stringstream ss;
        ss << "0x"; // prepend 0x
        ss << hex << uppercase << setfill('0');
        for (int i = x.chunks.size() - 1; i >= 0; --i)
        {
            if (i == (int)x.chunks.size() - 1)
                ss << std::hex << uppercase << x.chunks[i];
            else
                ss << setw(8) << x.chunks[i];
        }
        return ss.str();
    }

    // Convert decimal string to BigInt
    static BigInt from_decimal(const string &s)
    {
        BigInt result;
        for (char c : s)
        {
            if (c < '0' || c > '9')
                continue;
            int digit = c - '0';
            result = BigInt::mul(result, BigInt(10));    
            result = BigInt::add(result, BigInt(digit));
        }
        return result;
    }

    static string to_decimal_Optimized(const BigInt &x)
    {
        if (x.is_zero())
            return "0";

        const uint32_t DEC_BASE = 1000000000;
        BigInt temp = x;
        vector<uint32_t> decimal_chunks;

        // Repeatedly divide by 1e9 instead of 10
        while (!temp.is_zero())
        {
            BigInt q, r;
            r = divideWithRemainder(temp, BigInt(DEC_BASE), q);
            decimal_chunks.push_back(r.chunks.empty() ? 0 : r.chunks[0]);
            temp = q;
        }

        // Convert chunks to string
        string result = to_string(decimal_chunks.back()); // Most significant chunk
        for (int i = (int)decimal_chunks.size() - 2; i >= 0; --i)
        {
            string part = to_string(decimal_chunks[i]);
            // Pad with leading zeros to maintain 9 digits
            result += string(9 - part.length(), '0') + part;
        }

        return result;
    }

    // Convert BigInt to decimal string
    static string to_decimal(const BigInt &x)
    {
        if (x.is_zero())
            return "0";
        BigInt temp = x;
        string s;
        BigInt ten(10);
        while (!temp.is_zero())
        {
            BigInt r = mod(temp, ten);
            s.push_back('0' + r.chunks[0]);
            temp = divideByTen(temp);
        }
        reverse(s.begin(), s.end());
        return s;
    }

    // Helper function to divide BigInt by 10
    static BigInt divideByTen(BigInt a)
    {
        BigInt result;
        result.chunks.resize(a.chunks.size());
        uint64_t carry = 0;
        for (int i = a.chunks.size() - 1; i >= 0; --i)
        {
            uint64_t cur = (carry << 32) | a.chunks[i];
            result.chunks[i] = (uint32_t)(cur / 10);
            carry = cur % 10;
        }
        result.normalize();
        return result;
    }

    // Compare two BigInts: returns -1 if a < b, 0 if a == b, 1 if a > b
    static int compare(const BigInt &a, const BigInt &b)
    {
        if (a.chunks.size() != b.chunks.size())
        {
            return a.chunks.size() < b.chunks.size() ? -1 : 1;
        }
        for (int i = a.chunks.size() - 1; i >= 0; --i)
        {
            if (a.chunks[i] != b.chunks[i])
            {
                return a.chunks[i] < b.chunks[i] ? -1 : 1;
            }
        }
        return 0; // They are equal
    }

    // a + b
    static BigInt add(const BigInt &a, const BigInt &b)
    {
        BigInt result;
        size_t n = max(a.chunks.size(), b.chunks.size());
        result.chunks.resize(n);
        uint64_t carry = 0;
        for (size_t i = 0; i < n; ++i)
        {
            uint64_t a_chunk = (i < a.chunks.size() ? a.chunks[i] : 0);
            uint64_t b_chunk = (i < b.chunks.size() ? b.chunks[i] : 0);
            uint64_t sum = a_chunk + b_chunk + carry;
            result.chunks[i] = (uint32_t)(sum & 0xFFFFFFFFu);
            carry = sum >> 32; // Get the carry for the next chunk
        }
        if (carry)
        {
            result.chunks.push_back((uint32_t)(carry));
        }
        return result;
    }

    // a - b
    static BigInt subtract(const BigInt &a, const BigInt &b)
    {
        if (compare(a, b) < 0)
        {
            throw runtime_error("Subtraction would result in a negative value");
        }

        BigInt result;
        result.chunks.resize(a.chunks.size());
        int64_t borrow = 0;
        for (size_t i = 0; i < a.chunks.size(); ++i)
        {
            int64_t a_chunk = (i < a.chunks.size() ? a.chunks[i] : 0);
            int64_t b_chunk = (i < b.chunks.size() ? b.chunks[i] : 0);
            int64_t diff = a_chunk - b_chunk + borrow;
            if (diff < 0)
            {
                diff += (1LL << 32);
                borrow = -1;
            }
            else
            {
                borrow = 0;
            }
            result.chunks[i] = (uint32_t)(diff);
        }
        result.normalize();
        return result;
    }

    // single-bit right shift: x /= 2
    void shr1()
    {
        uint32_t carry = 0;
        for (int i = (int)chunks.size() - 1; i >= 0; --i)
        {
            uint64_t cur = (uint64_t(chunks[i]) | (uint64_t(carry) << 32));
            chunks[i] = uint32_t(cur >> 1);
            carry = uint32_t(cur & 1u);
        }
        normalize();
    }

    // static helper: single-bit right shift
    static BigInt shr1(const BigInt &x)
    {
        BigInt y = x;
        y.shr1();
        return y;
    }

    // helper: is_even
    static bool is_even(const BigInt &x)
    {
        if (x.is_zero())
            return true;
        return (x.chunks[0] & 1) == 0;
    }

    // helper: is_one
    static bool is_one(const BigInt &x)
    {
        return (x.chunks.size() == 1 && x.chunks[0] == 1);
    }

    // k-bit left shift
    BigInt shlBits(size_t k) const
    {
        if (is_zero())
            return BigInt();
        BigInt result;
        size_t chunkshift = k / 32;
        uint32_t bitShift = uint32_t(k % 32);
        result.chunks.assign(chunkshift, 0);
        uint64_t carry = 0;
        for (size_t i = 0; i < chunks.size(); ++i)
        {
            uint64_t cur = (uint64_t)chunks[i] << bitShift;
            cur |= carry;
            result.chunks.push_back(uint32_t(cur & 0xffffffffu));
            carry = cur >> 32;
        }
        if (carry)
            result.chunks.push_back(uint32_t(carry));
        return result;
    }

    // bit length (0 for zero)
    size_t bitLength() const
    {
        if (is_zero())
            return 0;
        uint32_t msw = chunks.back();
        size_t bits = (chunks.size() - 1) * 32;
        bits += 32 - __builtin_clz(msw);
        return bits;
    }

    // normal multiplication a * b
    static BigInt mul(const BigInt &a, const BigInt &b)
    {
        if (a.is_zero() || b.is_zero())
            return BigInt(0);

        BigInt result;
        result.chunks.resize(a.chunks.size() + b.chunks.size(), 0);

        for (size_t i = 0; i < a.chunks.size(); ++i)
        {
            uint64_t carry = 0;
            uint64_t a_chunk = a.chunks[i];

            for (size_t j = 0; j < b.chunks.size(); ++j)
            {
                uint64_t cur = (uint64_t)result.chunks[i + j] +
                               a_chunk * (uint64_t)b.chunks[j] +
                               carry;
                result.chunks[i + j] = (uint32_t)(cur & 0xFFFFFFFFu);
                carry = cur >> 32;
            }

            size_t idx = i + b.chunks.size();
            while (carry > 0)
            {
                uint64_t cur = (uint64_t)result.chunks[idx] + carry;
                result.chunks[idx] = (uint32_t)(cur & 0xFFFFFFFFu);
                carry = cur >> 32;
                idx++;
            }
        }

        result.normalize();
        return result;
    }

    static BigInt mod(const BigInt &a, const BigInt &m)
    {
        BigInt res = a;
        while (compare(res, m) >= 0)
            res = subtract(res, m);
        while (compare(res, BigInt(0)) < 0)
            res = add(res, m);
        return res;
    }

    static BigInt modSafe(const BigInt &a, const BigInt &m)
    {
        if (compare(a, m) < 0)
            return a; // Already less than modulus

        BigInt res; // result
        res.chunks.resize(0);

        // Process input 'a' from most significant chunk to least
        for (int i = (int)a.chunks.size() - 1; i >= 0; --i)
        {
            // res = res * 2^32 + a.chunks[i]
            res = res.shlBits(32);
            res = add(res, BigInt(a.chunks[i]));

            // Reduce while res >= m
            while (compare(res, m) >= 0)
            {
                res = subtract(res, m);
            }
        }

        return res;
    }

    // Helper: divide a by b, return remainder, also sets quotient q
    static BigInt divideWithRemainder(const BigInt &a, const BigInt &b, BigInt &q)
    {
        if (compare(a, b) < 0)
        {
            q = BigInt(0);
            return a;
        }

        BigInt dividend = a;
        BigInt divisor = b;
        BigInt quotient(0);

        int shift = int(dividend.bitLength() - divisor.bitLength());
        divisor = divisor.shlBits(shift);

        for (; shift >= 0; shift--)
        {
            if (compare(dividend, divisor) >= 0)
            {
                dividend = subtract(dividend, divisor);
                quotient = quotient.add(quotient, BigInt(1).shlBits(shift));
            }
            divisor = BigInt::shr1(divisor);
        }

        q = quotient;
        return dividend; // remainder
    }

    // modular addition; a + b (mod m)
    static BigInt modAdd(const BigInt &a, const BigInt &b, const BigInt &mod)
    {
        BigInt sum = BigInt::add(a, b);
        BigInt modVal = BigInt::modSafe(sum, mod);
        return modVal;
    }

    // helper: modular subtraction: (a - b) % m
    static BigInt modSub(const BigInt &a, const BigInt &b, const BigInt &m)
    {
        BigInt diff;
        if (compare(a, b) >= 0)
        {
            diff = subtract(a, b);
        }
        else
        {
            diff = subtract(b, a);
            diff = subtract(m, diff);
        }
        if (compare(diff, m) >= 0)
            diff = subtract(diff, m);
        return diff;
    }

    // modular multiplication ; a * b (mod m)
    static BigInt modMul(const BigInt &a, const BigInt &b, const BigInt &mod)
    {
        BigInt product = BigInt::mul(a, b);
        return BigInt::modSafe(product, mod);
    }

    static BigInt modFast(const BigInt &a, const BigInt &m)
    {
        BigInt q;
        BigInt r = divideWithRemainder(a, m, q);
        return r;
    }

    // modular inverse using binary extended GCD
    static BigInt modInverse(BigInt a, const BigInt &m)
    {
        if (m.is_zero())
        {
            cout << "\n=== Modular Inverse Test ===\n";
            cout << "a = " << BigInt::to_decimal(a) << endl;
            cout << "m = " << BigInt::to_decimal(m) << endl;
            throw std::invalid_argument("Modular Inverse requires non-zero modulus");
        }

        a = mod(a, m);
        if (a.is_zero())
        {
            cout << "\n=== Modular Inverse Test ===\n";
            cout << "a = " << BigInt::to_decimal(a) << endl;
            cout << "m = " << BigInt::to_decimal(m) << endl;
            throw std::runtime_error("Modular inverse does not exist");
        }

        BigInt u = a;
        BigInt v = m;
        BigInt r(1); // coefficient for u
        BigInt s(0); // coefficient for v

        while (!u.is_zero())
        {
            while (is_even(u))
            {
                u = shr1(u);
                if (!is_even(r))
                    r = add(r, m); // r += m
                r = shr1(r);
            }

            while (is_even(v))
            {
                v = shr1(v);
                if (!is_even(s))
                    s = add(s, m); // s += m
                s = shr1(s);
            }

            if (compare(u, v) >= 0)
            {
                u = subtract(u, v);
                r = modSub(r, s, m);
            }
            else
            {
                v = subtract(v, u);
                s = modSub(s, r, m);
            }
        }

        // v is now gcd(a, m)
        if (compare(v, BigInt(1)) != 0)
        {
            cout << "\n=== Modular Inverse Test ===\n";
            cout << "a = " << BigInt::to_decimal(a) << endl;
            cout << "m = " << BigInt::to_decimal(m) << endl;
            throw std::runtime_error("inverse does not exist; gcd(a,m) != 1");
        }

        // s is the modular inverse
        return mod(s, m);
    }
};

// ---------------- Helpers ----------------

// Ask user if input is decimal or hex, then parse accordingly
BigInt inputNumber(const string &prompt)
{
    int type;
    cout << prompt << "\nSelect input type:\n";
    cout << "1. Decimal\n2. Hexadecimal\nEnter choice: ";
    cin >> type;
    string num;
    cout << "Enter number: ";
    cin >> num;
    if (type == 1)
        return BigInt::from_decimal(num);
    else
        return BigInt::from_hex(num);
}

// Display BigInt in decimal and hex
void printNumber(const BigInt &n)
{
    cout << "Decimal: " << BigInt::to_decimal(n) << endl;
    cout << "Hex:     " << BigInt::to_hex(n) << endl;
}

void printChunks(const BigInt &n)
{
    for (size_t i = 0; i < n.chunks.size(); ++i)
        cout << "chunks[" << i << "] = 0x"
             << hex << uppercase << n.chunks[i] << dec << endl;
}

BigInt generateModulus(size_t n_bits)
{
    random_device rd;
    mt19937_64 gen(rd());
    BigInt result;
    size_t num_chunks = (n_bits + 31) / 32;
    result.chunks.resize(num_chunks);
    for (size_t i = 0; i < num_chunks; ++i)
        result.chunks[i] = gen();
    // Ensure exactly n_bits
    size_t extra_bits = (32 * num_chunks) - n_bits;
    if (extra_bits > 0)
    {
        result.chunks.back() &= ((1u << (32 - extra_bits)) - 1);
        result.chunks.back() |= (1u << (31 - extra_bits));
    }
    result.normalize();
    return result;
}

// ---------------- Test Functions ----------------
void testRepresentation()
{
    cout << "\n=== Number Representation Test ===\n";
    BigInt n = inputNumber("Enter a large number");
    // printNumber(n);
    cout << "Bit length: " << n.bitLength() << endl;
    cout << "Number of chunks: " << n.chunks.size() << endl;
    printChunks(n);
}

BigInt getModulusInteractive()
{
    int choice;
    cout << "Select modulus type:\n";
    cout << "1. Enter modulus directly\n";
    cout << "2. Generate n-bit modulus\n";
    cout << "Enter choice: ";
    cin >> choice;

    if (choice == 1)
    {
        return inputNumber("Enter modulus m");
    }
    else if (choice == 2)
    {
        size_t n_bits;
        cout << "Enter modulus bit length n: ";
        cin >> n_bits;
        return generateModulus(n_bits);
    }
    else
    {
        cout << "Invalid choice. Defaulting to modulus = 1.\n";
        return BigInt(1);
    }
}

void testModularReduction()
{
    cout << "\n=== Modular Reduction Test ===\n";
    BigInt a = inputNumber("Enter number a");
    BigInt m = getModulusInteractive();
    BigInt r = BigInt::modSafe(a, m);
    cout << "a % m = ";
    cout << "Hex: " << BigInt::to_hex(r) << endl;
    cout << "Decimal: " << BigInt::to_decimal_Optimized(r) << endl;
}

void testModularAddition()
{
    cout << "\n=== Modular Addition Test ===\n";

    // get inputs
    BigInt a = inputNumber("Enter number a");
    BigInt b = inputNumber("Enter number b");
    BigInt m = getModulusInteractive();

    // perform modular addition
    BigInt r = BigInt::modAdd(a, b, m);

    cout << "\n=== Modular Addition Test ===\n";
    cout << "(a + b) % m = ";
    cout << "Hex: " << BigInt::to_hex(r) << endl;
    cout << "Decimal: " << BigInt::to_decimal_Optimized(r) << endl;
}

void testModularMultiplication()
{
    cout << "\n=== Modular Multiplication Test ===\n";
    BigInt a = inputNumber("Enter number a");
    BigInt b = inputNumber("Enter number b");
    BigInt m = getModulusInteractive();
    BigInt r = BigInt::modMul(a, b, m);

    cout << "(a * b) % m = ";
    cout << "Hex: " << BigInt::to_hex(r) << endl;
    cout << "Decimal: " << BigInt::to_decimal_Optimized(r) << endl;
}

void testModularInverse()
{
    cout << "\n=== Modular Inverse Test ===\n";
    BigInt a = inputNumber("Enter number a");
    BigInt m = getModulusInteractive();
    BigInt inv = BigInt::modInverse(a, m);
    cout << "\n=== Modular Inverse Test ===\n";
    cout << "a^-1 mod m = ";
    cout << "Hex: " << BigInt::to_hex(inv) << endl;
    cout << "Decimal: " << BigInt::to_decimal_Optimized(inv) << endl;
}

void testNormalAddition()
{
    cout << "\n=== Normal Addition Test ===\n";
    BigInt a = inputNumber("Enter number a");
    BigInt b = inputNumber("Enter number b");
    BigInt r = BigInt::add(a, b);
    cout << "a + b = ";
    cout << "Hex: " << BigInt::to_hex(r) << endl;
    cout << "Decimal: " << BigInt::to_decimal(r) << endl;
}

// ---------------- Main Menu ----------------
int main()
{
    int choice;
    do
    {
        cout << "\n==== BigInt Test Menu ====\n";
        cout << "1. Number Representation (show chunks)\n";
        cout << "2. Modular Reduction (a % m)\n";
        cout << "3. Modular Addition ((a+b) % m)\n";
        cout << "4. Modular Multiplication ((a*b) % m)\n";
        cout << "5. Modular Inverse (a^-1 mod m)\n";
        cout << "6. Normal Addition (a + b)\n";
        cout << "0. Exit\n";
        cout << "Enter choice: ";
        cin >> choice;
        switch (choice)
        {
        case 1:
            testRepresentation();
            break;
        case 2:
            testModularReduction();
            break;
        case 3:
            testModularAddition();
            break;
        case 4:
            testModularMultiplication();
            break;
        case 5:
            testModularInverse();
            break;
        case 6:
            testNormalAddition();
            break;
        case 0:
            cout << "Exiting.\n";
            break;
        default:
            cout << "Invalid choice.\n";
            break;
        }
    } while (choice != 0);

    return 0;
}
