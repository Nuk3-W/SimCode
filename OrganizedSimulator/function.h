#ifndef Function_H
#define Function_H

#include <iostream>
#include <cmath>
#include <cctype>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>   


using namespace std;

string HexToBin(string currentaddresshex);
int BinaryToDecimal(int n);
string DecToBinary(int num);
string HashBinToDec(string binstring);
unsigned long int HashFunction(string address, int BufferSize);

string HexToBin(string currentaddresshex)
{
    string bincurrentaddress;
    size_t i = 0;
    while (currentaddresshex[i]) {
 
        switch (currentaddresshex[i]) {
        case '0':
            bincurrentaddress += "0000";
            break;
        case '1':
            bincurrentaddress += "0001";
            break;
        case '2':
            bincurrentaddress += "0010";
            break;
        case '3':
            bincurrentaddress += "0011";
            break;
        case '4':
            bincurrentaddress += "0100";
            break;
        case '5':
            bincurrentaddress += "0101";
            break;
        case '6':
            bincurrentaddress += "0110";
            break;
        case '7':
            bincurrentaddress += "0111";
            break;
        case '8':
            bincurrentaddress += "1000";
            break;
        case '9':
            bincurrentaddress += "1001";
            break;
        case 'A':
        case 'a':
            bincurrentaddress += "1010";
            break;
        case 'B':
        case 'b':
            bincurrentaddress += "1011";
            break;
        case 'C':
        case 'c':
            bincurrentaddress += "1100";
            break;
        case 'D':
        case 'd':
            bincurrentaddress += "1101";
            break;
        case 'E':
        case 'e':
            bincurrentaddress += "1110";
            break;
        case 'F':
        case 'f':
            bincurrentaddress += "1111";
            break;
        default:
            cout << "\nInvalid hexadecimal digit "
                 << currentaddresshex[i];
        }
        i++;
    }
        while  (bincurrentaddress.length() != 32)
    {
        bincurrentaddress.insert(0, 1, '0');
    }
    return bincurrentaddress;
};
int BinaryToDecimal(int n) 
{
    int num = n;
    int dec_value = 0; 
        // Initializing base value to 1, i.e 2^0
    int base = 1;

    int temp = num;
    while (temp) {
        int last_digit = temp % 10;
        temp = temp / 10;

        dec_value += last_digit * base;

        base = base * 2;
    }
    return dec_value;
}; 
//this is just taken from stack overflow
// function to convert decimal to binary
string DecToBinary(int num)
{   
    int count = 0;
    //cout << num << " ";
    string str;
      while(num && count < 32){
      if(num & 1) // 1
        str = '1' + str;
      else // 0
        str = '0' + str;
      num>>=1; // Right Shift by 1 
      count ++;
    } 
    //cout << str << endl;
    
    return str;
}
string HashBinToDec (string binstring) 
{
    int decimalNumber = 0;
    int length = binstring.length();

    for (int i = 0; i < length; ++i) {
        if (binstring[i] == '1') {
            decimalNumber += pow(2, length - 1 - i);
        }
    }
    return to_string(decimalNumber);
};
unsigned long int HashFunction (string address, int BufferSize)
{
    unsigned long int Index = stoul(HashBinToDec(address));  
    Index = Index % BufferSize; //indexs the address key to fit the index table 
    return Index;  
};
//this is from geeks for geeks //This thing was backwards but fixed
#endif