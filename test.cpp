#include <iostream>
#include <vector>
#include <string>
#include <cassert>
using namespace std;

// Assume your BigInt class and modAdd function are defined above

struct TestCase {
    string a;
    string b;
    string mod;
    string expected; // expected result as decimal
};

void runModAddTests() {
    vector<TestCase> tests = {
        {"1", "1", "5", "2"},
        {"2", "3", "5", "0"},
        {"4", "1", "2", "1"},
        {"123456789012345678", "987654321098765432", "1000000000000000000", "111111110111111110"},
        {"0", "7", "3", "1"},
        {"5", "0", "3", "2"},
        {"0", "0", "7", "0"},
        {"4294967301", "4294967310", "8589934592", "15"} // 2^32+5 + 2^32+10 mod 2^33
    };

    for (size_t i = 0; i < tests.size(); ++i) {
        BigInt a = BigInt::from_decimal(tests[i].a);
        BigInt b = BigInt::from_decimal(tests[i].b);
        BigInt m = BigInt::from_decimal(tests[i].mod);
        BigInt expected = BigInt::from_decimal(tests[i].expected);

        BigInt result = BigInt::modAdd(a, b, m);

        if (BigInt::compare(result, expected) == 0) {
            cout << "Test " << i+1 << " PASSED." << endl;
        } else {
            cout << "Test " << i+1 << " FAILED!" << endl;
            cout << "a = " << tests[i].a << ", b = " << tests[i].b << ", mod = " << tests[i].mod << endl;
            cout << "Expected: " << tests[i].expected << ", Got: " << BigInt::to_decimal(result) << endl;
        }
    }
}

int main() {
    runModAddTests();
    return 0;
}

