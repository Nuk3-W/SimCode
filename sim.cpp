#include <iostream>
#include <cmath>
#include <cctype>
#include <vector>
#include <string>
#include <sstream>

using namespace std;

int BinaryToDecimal(int);
string HexToBin(string);
string DecToBinary(int);

class Cache
{
public:
    Cache * LowerLevel;
    int Size; // In bytes
    int Type; // This is ow many ways are in each set Type 1 = Direct Mapping everything else is set associative and associative mapping
    int BlockSize; //Number of Bytes per block
    int Blocks; //Number of blocks
    int Sets; //Number of sets
    int LSBForSetIndex; //amount of least significant bits needed for the Block Index after the LSBForByte bits
    int TagBits; //amount of bits allocated for the tag after block is written in the cache
    int ExtraBits; 
    int LRUCounterBits;
    int Hits = 0;
    int Misses = 0;
    int* TrueWay = new int; //after the tag address is matched with a way, the true way holds the value so we can access that specific way
    string* TagPoint = new string; //pointer for storing only the current tag of the current read/write address
    int* SetOffsetPoint = new int; //pointer for storing the current Set Offset of the address from the read/write address
    vector<vector<string>> VirtualMemory;
    vector<vector<string>> TagMemory; //tag memory to confirm block identity
    Cache (int size, int type, int blocksize, Cache * lowerlevel) // cache class constructor
    {   
        LowerLevel = lowerlevel;
        Size = size; 
        Type = type;
        BlockSize = blocksize;
        Blocks = size/blocksize;
        Sets = Blocks/Type;
        ExtraBits = ceil((log2(Type)+2)); //ExtraBits is the bits needed for LRUCounter, DirtyBit, and ValidBit
        LRUCounterBits = ceil(log2(Type)); 
        LSBForSetIndex = ceil(log2(Sets));
        TagBits = (32 - LSBForSetIndex);
        Initialize();
    };
    void Initialize () //creates the array that models the cache along with the tag array that confirms its location
    { 
        int m = Sets, n = Type, o = (BlockSize * 8); // o is if I had actual data inside the memory vector
        VirtualMemory.resize(m); //first layer of virtual memory is set
        for (int i = 0; i < m; ++i)
        {
            VirtualMemory[i].resize(n); //second layer is the way
            for (int j = 0; j < n; j++)
            {
                VirtualMemory[i][j].resize((ExtraBits), '0'); //this makes a string of every bit based on the blocksize and the extra bits needed for the counter, valid bit, and dirt bit
            };
        };
        TagMemory.resize(m); //first layer of tag memory is set
        for (int i = 0; i < m; ++i)
        {
            TagMemory[i].resize(n); //second layer is the amount of ways
            for (int j = 0; j < n; j++)
            {
                TagMemory[i][j].resize(TagBits, '0'); //this makes a string as a placeholder for the tag 
            };
        };
    };
    string Read (string bincurrentaddress)
    {   
        AddressDecoder(bincurrentaddress);
        bool ValidTag = TagChecker(); //false if no tag matches the address tag, true otherwise
        bool ValidBit = BlockValidityChecker(bincurrentaddress);
        bool ValidDirty = DirtyBitChecker();
        if (ValidBit == true && ValidTag == true)
        {
                Hits ++; //= data; since I'm only doing blocks with no data IM not implementing the data rn
                LRUCounter();
                return *TagPoint;
        }
        else
        {   //this is the read miss
            int LRUWay = LRUChecker(); 
            Misses ++;
            Allocate(LRUWay, ValidDirty, bincurrentaddress);
            return *TagPoint;
        }
    };//for data I would add a string parameter and set the VirtualMemory equal to that at the correct block
    void Write (string bincurrentaddress) //NOT FINISHED BECAUSE READ FUNCTION NEEDED TO FINISH A WRITE MISS SINCE WE NEED TO READ FROM NEXT LEVEL OF MEMORY
    {   
        AddressDecoder(bincurrentaddress); //Obtains the decimal Line/Set value of the block as an integer and the Tag Value in binary as a string since the string is the main comparison for a block
        bool ValidTag = TagChecker(); //false if no tag matches the address, tag is true otherwise
        bool ValidBit = BlockValidityChecker(bincurrentaddress); 
        bool ValidDirty = DirtyBitChecker();
        if (ValidBit == true && ValidTag == true)
        {
                Hits ++; //= data; since I'm only doing blocks with no data IM not implementing the data rn
                LRUCounter();
        }
        else
        {
            int temp = LRUChecker();
            Misses ++; 
            Allocate(temp, ValidDirty, bincurrentaddress); //NOT IMPLEMENTED YET 
        }
    };
    bool BlockValidityChecker (string bincurrentaddress) //valid bit needs to be checked which indicates if next action is a miss or hit
    {
        if (VirtualMemory[*SetOffsetPoint][*TrueWay].substr(1) == "1") //if valid bit = 1 then true, otherwise false
        {
            return true;
        } 
        else
        {
            return false;
        }
    };
    void AddressDecoder(string bincurrentAddress) //basically says what set and way the current block should be looked for
    {   
        int SetOffset = 0;
        string BinTag;
        string BinSet;

        *TagPoint = bincurrentAddress.substr(0, (TagBits));
        bincurrentAddress.erase(0, (TagBits));
        BinSet = bincurrentAddress.substr(0, (LSBForSetIndex));
        bincurrentAddress.erase(0, (LSBForSetIndex));

        SetOffset = stoi(BinSet);

        *SetOffsetPoint = BinaryToDecimal(SetOffset);
    }
    bool TagChecker ()
    {
        for (int i = 0; i < TagMemory[*SetOffsetPoint].size(); i++) //checks tag
        {   
            if (*TagPoint == TagMemory[*SetOffsetPoint][i])
            {
                *TrueWay = i;
                return true;
            }
        };
        *TrueWay = 0;
        return false;
    }
    bool DirtyBitChecker()
    {
                if (VirtualMemory[*SetOffsetPoint][*TrueWay].substr(0) == "1") //if dirty bit = 1 then true, otherwise false (almost the same as valid checker)
        {
            return true;
        } 
        else
        {
            return false;
        }
    }
    void Allocate(int Temp, bool Dbit, string bincurrentaddress) //having the binary address as a parameter even after its decoded is so it can get redecoded
    //in the next write request to the next cache
    {   
        string ReadTag;
        if (LowerLevel != NULL) //if this doesnt have a lower level to write to, it's assumed that it will hit since it's main memory
            {
                if (Dbit == true) //this is the dirty bit 
                {
                    WriteBack(Temp, bincurrentaddress);
                    Dbit = false;
                }
                else
                {
                    LowerLevel->Read(bincurrentaddress);
                    TagMemory[*SetOffsetPoint][Temp] = *TagPoint; 
                }
            }
            else 
            {   
                TagMemory[*SetOffsetPoint][Temp] = *TagPoint;
            }
           /* if (Dbit == true) //this is the dirty bit 
            {
                WriteBack(Temp, bincurrentaddress);
                Dbit = false;
            }
            else
            {
                TagMemory[*SetOffsetPoint][*TrueWay] = LowerLevel->Read(bincurrentaddress);
            }*/
    };
    void WriteBack (int temp, string bincurrentaddress)
    {       
        if (LowerLevel != NULL)
        {
            LowerLevel->Write(bincurrentaddress);
        }
    };  
    int LRUChecker () //might rename. This checks which way was least used for the write/read allocation
    {
        int max = 0; //I dont think the counter will need more digit than this 
        int Way = 0;
        for (int i = 0; i < VirtualMemory[*SetOffsetPoint].size(); i++) //this is to check the LRUCounter of each block in the set to see which one should be evicted
        {   
            int temp = stoi(VirtualMemory[*SetOffsetPoint][i].substr(2, LRUCounterBits));
            if (temp > max)
            {
                max = temp;
                Way = i;
            }
        }
        return Way;
    };
    void LRUCounter () 
    {
        for ( int i = 0; i < Type; i++)
        {   
            int temp = stoi(VirtualMemory[*SetOffsetPoint][i].substr((BlockSize * 8) + 2, 4));
            temp = BinaryToDecimal(temp);
            temp ++;
            string str = DecToBinary(temp);
        VirtualMemory[*SetOffsetPoint][i].substr((BlockSize * 8) + 2, 4) = str;

        }
        VirtualMemory[*SetOffsetPoint][*TrueWay].substr((BlockSize * 8) + 2, 4) = "0000";

    



    }
    
};

 /* int main ()
{   
    string temp;
    int Size = 8192;
    int Type = 4;
    int BlockSize = 32;
    string CurrentAddressHex = "400341a0";
    temp = HexToBin(CurrentAddressHex);
    cout << "Input cache size, type, and block size:" << endl;
    Cache L2 = Cache(Size, Type, BlockSize, NULL); //different

    Size = 262144;
    Type = 8;
    Cache L1 = Cache(Size, Type, BlockSize, &L2); 
    
    L1.Write("01000000000000110100000110100000");
    L1.Read("00000000110111111100111110101000");


    cout << L1.Misses << endl;
    cout << L1.Hits << endl;
    return 0;
    
} */

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
    string str;
      while(num){
      if(num & 1) // 1
        str+='1';
      else // 0
        str+='0';
      num>>=1; // Right Shift by 1 
    } 
    return str;
}
//this is from geeks for geeks
