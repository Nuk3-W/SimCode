#include <iostream>
#include <cmath>
#include <cctype>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

using namespace std;

struct MarkovEntry;

int BinaryToDecimal(int);
string HexToBin(string);
string DecToBinary(int);  
unsigned long int HashFunction (string, int);
string HashBintoDec (string); 
bool CompareByValue(const MarkovEntry& a, const MarkovEntry& b);


class GHB;
class IT;
class GHBEntry;
class Cache;
class VictimCache;
class PrefetchBuffer;

struct GHBEntry
{   
    string Address;
    GHBEntry* Next = nullptr;
    GHBEntry* Previous = nullptr;
    GHBEntry* Duplicate = nullptr;
    int HashStep = 0;
    GHBEntry(string addr) : Address(addr), Next(nullptr) {} //also this is apparently a better way to assign than directly in the constructor
};

struct IT 
{   
    GHB* Connection;
    vector<GHBEntry*> IndexTable; 
    size_t BufferSize;
    int BlockSize;
    IT (size_t buffersize, int blocksize, GHB* connection) : BufferSize(buffersize), BlockSize(blocksize), Connection(connection) 
    {
        IndexTable.resize(BufferSize, nullptr); //not sure if this executes before the buffersize is given a proper size
    };
    void LookUp (GHBEntry* entry) 
    {   
        int BlockBits = (ceil(log2(BlockSize)));
        //if (entry->Address.length() == 32)
        //    TempAddr = entry->Address.substr((entry->Address.length() - BlockBits), BlockBits); 
        int Index = HashFunction(entry->Address, BufferSize); //checkk if the hash function takes in a string or int
        /*if (Index == 416)
        {
            cout << "Hi" << endl;
        }; */
        if (IndexTable[Index] == nullptr)
        {
            IndexTable[Index] = entry;
        }
        else if (IndexTable[Index]->Address == entry->Address)
        {   
            entry->Duplicate = IndexTable[Index];
            IndexTable[Index] = entry;
        }
        else
        {
            while(IndexTable[Index] != nullptr) //here we iterate through the linked list to see if the address has been indexed with collisions
            {
                if(IndexTable[Index]->Address == entry->Address) 
                {
                    entry->Duplicate = IndexTable[Index];
                    IndexTable[Index] = entry;
                    return;
                }
                if (Index != 511)
                {
                    Index ++;
                }
                else
                    Index = 0;
                entry->HashStep ++;
                //Temp = Current;
                //Current = Current->ITLookUp;
            };
            //Temp->ITLookUp = entry; //not sure if this will work because the previos node in the it might not point to it I added the temp thing to possibly fix idk
        }
        IndexTable[Index] = entry; 
    };
    void DeletionOfDuplicate (GHBEntry* entry)
    {
        int Index = HashFunction(entry->Address, BufferSize);
        Index += (entry->HashStep); 
        if (Index > 511)
            Index = Index - 512; //the hashstep was going over the buffersize of the table
        /*if (Index == 416)
        {
            cout << "Hi" << endl;
        }; */
        if (IndexTable[Index]->Address == entry->Address)
        {
            GHBEntry* Current = IndexTable[Index];
            if (Current == entry) //this is used when the head of the ghb is also the first instance of itself in the ghb
            {
                IndexTable[Index] = nullptr;
                return;
            }
            while (Current->Duplicate != entry)
            {
                Current = Current->Duplicate;
            };
            Current->Duplicate = nullptr;
        }
        else
        {
            while (IndexTable[Index]->Address != entry->Address)
            {
                Index ++;
            };
            GHBEntry* Current = IndexTable[Index];
            while (Current->Duplicate != entry)
            {
                Current = Current->Duplicate;
            };
            Current->Duplicate = nullptr;
        }
    };
};
class GHB
{
public:
    struct MarkovEntry
    {
        int Counter = 0;
        string Address = "0"; 
    };  
    IT* Connection;
    static bool CompareByValue(const MarkovEntry& a, const MarkovEntry& b); 
    GHBEntry* Head = nullptr;
    GHBEntry* Tail = nullptr;
    size_t BufferSize = 0; //apparently I should use this when counting size instead of unsigned int idk
    size_t Count = 0;
    GHB (size_t buffersize) : BufferSize(buffersize)
    {
    };
    void ConnectToIT (IT* connection) //was gonna be a circular dependency for IT and GHB so added assign fucntion
    {
        Connection = connection;
    }
    GHBEntry* Insert (string missaddress, int blockbits)
    {    
        GHBEntry* NewEntry = new GHBEntry(missaddress); //this is a pointer the heap keep the memory even after pointer is pushed from ghb
        if (Count != BufferSize)
            Count ++;
        else 
        {
            GHBEntry* OldHead = Head;
            Head = Head->Next;
            Connection->DeletionOfDuplicate(OldHead); //used to delete pointers that werent updated after deletion
            //Tail->Next = Tail; 
            delete OldHead;
        }
        if (Tail != nullptr)
        {   
            Tail->Next = NewEntry;
            NewEntry->Previous = Tail;
            Tail = NewEntry; 
        }
        else
        {   
            Head = NewEntry;
            Tail = NewEntry;
            NewEntry->Next = NewEntry;
            NewEntry->Previous = NewEntry;
        }
        return NewEntry;
/*        if (NewEntry->Duplicate == nullptr)
        {
            for (int i = 0; i < Count; i++) //goes backwards through the circular buffer and finds the most recent duplicate 
            {   
                GHBEntry* Checker = NewEntry;
                if (Checker->Previous->Address != NewEntry->Address)
                    Checker = Checker->Previous;
                else
                {
                    NewEntry->Duplicate = Checker;
                    break;
                }
            }
        } */ //not needed because thats the point of the index
        /*if (Count == BufferSize)
        {
            Tail->Next = Head;
            Head = Head->Next;
        }
        return NewEntry; */
    };
    struct CompareByCounter 
    {
        bool operator()(const MarkovEntry& a, const MarkovEntry& b) const //chatgpt generated no clue about the sort function cool though
        {
            return a.Counter > b.Counter;
        }
    };
    vector<string> MarkovPredictor(GHBEntry* missreference) 
    {   
        vector<MarkovEntry> MarkovTable;
        MarkovTable.resize(512); 
        GHBEntry* Current = missreference->Duplicate; 
        vector<string> Max; //this is to keep track of the most entries after the miss address
        Max.resize(16, "0");
        if (Current == nullptr)
        {
            return Max;
        }
        while (Current != nullptr)
        {   
            int temp = HashFunction(Current->Next->Address, 512); //gets the index based on the hash function            cout << MarkovTable[temp].Address << endl;
            if (MarkovTable[temp].Address == "0")
            {
                MarkovTable[temp].Counter ++;
                MarkovTable[temp].Address = Current->Next->Address;
            }
            else if (MarkovTable[temp].Address == Current->Next->Address)
            {   
                //cout << MarkovTable[temp].Address << endl;
                MarkovTable[temp].Counter ++;
            }
            else
            {
                while (MarkovTable[temp].Address != Current->Next->Address)
                {   
                    if (MarkovTable[temp].Address == "0") //this checks if we hit a blank entry 
                    {
                        MarkovTable[temp].Address = Current->Next->Address;
                        MarkovTable[temp].Counter ++;
                        break;
                    }
                    else if (MarkovTable[temp].Address == Current->Next->Address)
                    {
                        MarkovTable[temp].Counter ++;
                        break;
                    }
                    else if (temp == 512)
                    {
                        temp = 0;
                    }
                    else
                    {
                        temp ++; //for correction im just using a sequential search after the hash
                    }
                }
            }
            Current = Current->Duplicate;
        }
        sort(MarkovTable.begin(), MarkovTable.end(), CompareByCounter()); //no idea you could do this and as such it isnt my code chatgpt generated
        for (int i = 0; i < Max.size(); i++)  //iterate 16 times to get the highest probable next addresses based on markov predictor
        {
            Max[i] = MarkovTable[i].Address;
        };
        return Max;
    };
};


class Cache
{
public:
    Cache* LowerLevel;
    Cache* VCPtr = NULL; 
    Cache* Connection = NULL;
    GHB* GHBPtr = nullptr;  
    IT* IndexTable = nullptr; 
    Cache* PrefetchBuffer = nullptr; 
    string Name;
    bool* SwapHitCheck = new bool(false);
    bool* PrefetchDupe = new bool(false);
    int SwapHitCounter = 0;
    int Size; // In bytes
    int Type; // This is ow many ways are in each set Type 1 = Direct Mapping everything else is set associative and associative mapping
    int BlockSize; //Number of Bytes per block
    int Blocks; //Number of blocks
    int Sets; //Number of sets
    int LSBForSetIndex; //amount of least significant bits needed for the Block Index after the LSBForByte bits
    int TagBits; //amount of bits allocated for the tag after block is written in the cache
    int ExtraBits; 
    int LRUCounterBits;
    int BlockOffsetBits;
    int Hits = 0;
    int Misses = 0;
    int WriteMiss = 0;
    int ReadMiss = 0;
    int Reads = 0;
    int Writes = 0;
    int WriteBacks = 0;
    int SwapRequests = 0;
    int PrefetchSaves = 0;
    string* WriteBackAddress; //to properly do a write back, the old address of the eviction block is needed not the current therefore we use this pointer to reconstrcut it
    int* TrueWay = new int; //after the tag address is matched with a way, the true way holds the value so we can access that specific way
    string* TagPoint = new string; //pointer for storing only the current tag of the current read/write address
    int* SetOffsetPoint = new int; //pointer for storing the current Set Offset of the address from the read/write address
    vector<vector<string>> VirtualMemory; 
    vector<vector<string>> TagMemory; //tag memory to confirm block identity
    Cache (Cache* connection, string name, int size, int type, int blocksize, Cache * lowerlevel) // cache class constructor
    {   
        Name = name;
        if (Name[0] = 'V')
            Connection = connection;
        LowerLevel = lowerlevel;
        Size = size; 
        Type = type;
        BlockSize = blocksize;
        Blocks = size/blocksize;
        Sets = Blocks/Type;
        ExtraBits = ceil((log2(Type)+2)); //ExtraBits is the bits needed for LRUCounter, DirtyBit, and ValidBit
        LRUCounterBits = ceil(log2(Type)); 
        LSBForSetIndex = ceil(log2(Sets));
        BlockOffsetBits = ceil(log2(BlockSize));
        TagBits = (32 - LSBForSetIndex - BlockOffsetBits); 
        Initialize();
    };
    virtual void Initialize () //creates the array that models the cache along with the tag array that confirms its location
    { 
        int m = Sets, n = Type, o = (BlockSize * 8); // o is if I had actual data inside the memory vector
        VirtualMemory.resize(m); //first layer of virtual memory is set
        for (int i = 0; i < m; ++i)
        {
            VirtualMemory[i].resize(n); //second layer is the way
            for (int j = 0; j < n; j++)
            {
                VirtualMemory[i][j] = "00";
                string temp = DecToBinary(j); 
                while (temp.length() != LRUCounterBits)
                {
                    temp = '0' + temp;
                }
                VirtualMemory[i][j] = VirtualMemory[i][j] + temp;
                //cout << VirtualMemory[i][j] << endl;
            };
        };
        TagMemory.resize(m); //first layer of tag memory is set
        for (int i = 0; i < m; ++i)
        {
            TagMemory[i].resize(n); //second layer is the amount of ways
            for (int j = 0; j < n; j++)
            {
                TagMemory[i][j].resize(TagBits, '0'); //this makes a string as a placeholder for the tag
                //cout << TagMemory[i][j] << endl;
            };
        };
        //cout << TagMemory[0][0] << "Check" << endl;
    };
    void Assign (Cache* ptr, GHB* ghb, IT* indextable, Cache* prefetchbuffer)  
    {
        VCPtr = ptr;
        GHBPtr = ghb;
        IndexTable = indextable;
        PrefetchBuffer = prefetchbuffer;  
    }
    virtual bool Read (string bincurrentaddress)
    {   
        while (bincurrentaddress.length() != 32)
        {
            bincurrentaddress.insert(0, 1,'0');
        }
        AddressDecoder(bincurrentaddress);
        /*for (int i = 0; i < VirtualMemory[*SetOffsetPoint].size(); i++) 
        {
            cout << TagMemory[*SetOffsetPoint][i] << endl;
        }*/
        bool ValidTag = TagChecker(); //false if no tag matches the address tag, true otherwise
        bool ValidBit = BlockValidityChecker(*TrueWay);
        bool ValidDirty = DirtyBitChecker(*TrueWay);
        if (ValidBit == true && ValidTag == true)
        {
                Hits ++; //= data; since I'm only doing blocks with no data IM not implementing the data rn
                Reads ++;
                LRUCounter();
        }
        else
        {   //this is the read miss 
            *PrefetchDupe = false;
            int LRUWay = LRUChecker(); 
            Misses ++;
            ReadMiss ++;
            Reads ++;
            ValidDirty = DirtyBitChecker(LRUWay);
            *TrueWay = LRUWay;
            if (PrefetchBuffer != nullptr)
            {
                if (PrefetchBuffer->Read(bincurrentaddress))
                {
                    *PrefetchDupe = true;
                }
            };
            Allocate(LRUWay, ValidDirty, bincurrentaddress, PrefetchDupe); 
            LRUCounter();
            bool check = false;
            if (VCPtr != NULL)
            {
                check = *VCPtr->SwapHitCheck;
            }
            if (VCPtr == NULL)
            {
                VirtualMemory[*SetOffsetPoint][LRUWay][0] = '0';
            }
            else if (LowerLevel != NULL && check == false)
            {
                VirtualMemory[*SetOffsetPoint][LRUWay][0] = '0';
            }
            VirtualMemory[*SetOffsetPoint][LRUWay][1] = '1';
        }
        return false; 
    };//for data I would add a string parameter and set the VirtualMemory equal to that at the correct block
    virtual int Write (string bincurrentaddress) //NOT FINISHED BECAUSE READ FUNCTION NEEDED TO FINISH A WRITE MISS SINCE WE NEED TO READ FROM NEXT LEVEL OF MEMORY
    {   
        while (bincurrentaddress.length() != 32)
        {
            bincurrentaddress.insert(0, 1,'0');
        }
        AddressDecoder(bincurrentaddress); //Obtains the decimal Line/Set value of the block as an integer and the Tag Value in binary as a string since the string is the main comparison for a block
        /*for (int i = 0; i < VirtualMemory[*SetOffsetPoint].size(); i++) 
        {
            cout << TagMemory[*SetOffsetPoint][i] << endl;
        } */
        bool ValidTag = TagChecker(); //false if no tag matches the address, tag is true otherwise
        bool ValidBit = BlockValidityChecker(*TrueWay); 
        bool ValidDirty = DirtyBitChecker(*TrueWay);
        if (ValidBit == true && ValidTag == true)
        {
            Hits ++; //= data; since I'm only doing blocks with no data IM not implementing the data rn
            Writes ++;
            VirtualMemory[*SetOffsetPoint][*TrueWay][0] = '1'; //sets dirty bit to 
            LRUCounter();
            return 0;
        }
        else
        {
            *PrefetchDupe = false;
            int LRUWay = LRUChecker();
            Misses ++;
            WriteMiss ++;
            Writes ++;
            ValidDirty = DirtyBitChecker(LRUWay);
            *TrueWay = LRUWay;
            if (PrefetchBuffer != nullptr)
            {
                if (PrefetchBuffer->Read(bincurrentaddress))
                {
                    *PrefetchDupe = true;
                }
            };
            Allocate(LRUWay, ValidDirty, bincurrentaddress, PrefetchDupe); //NOT IMPLEMENTED YET
            LRUCounter();
            VirtualMemory[*SetOffsetPoint][LRUWay][1] = '1'; //sets valid bit to true
            VirtualMemory[*SetOffsetPoint][LRUWay][0] = '1'; //sets dirty bit to true
            return 1; 
        }
    };
    bool BlockValidityChecker (int Way) //valid bit needs to be checked which indicates if next action is a miss or hit
    {
        if (VirtualMemory[*SetOffsetPoint][Way][1] == '1') //if valid bit = 1 then true, otherwise false
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
        //cout << bincurrentAddress << " ";
        *TagPoint = bincurrentAddress.substr(0, (TagBits));
        //cout << *TagPoint << " ";
        bincurrentAddress.erase(0, (TagBits));
        BinSet = bincurrentAddress.substr(0, (LSBForSetIndex)); //possible issue if LSBFORSETINDEx is = 0
        //cout << BinSet << " ";
        bincurrentAddress.erase(0, (LSBForSetIndex));
        if (LSBForSetIndex != 0)
        {
        SetOffset = stoi(BinSet);

        *SetOffsetPoint = BinaryToDecimal(SetOffset);
        }
        else 
        *SetOffsetPoint = 0;
        //cout << *SetOffsetPoint << " ";
    }
    bool TagChecker ()
    {
        *TrueWay = 0;
        for (int i = 0; i < TagMemory[*SetOffsetPoint].size(); i++) //checks tag
        {   
            //cout << TagMemory[*SetOffsetPoint][i] << endl;
            //cout << i << " " << *TagPoint << " "<< TagMemory[*SetOffsetPoint][i] << endl;
            if (*TagPoint == TagMemory[*SetOffsetPoint][i])
            {
                *TrueWay = i;
                //cout << i << " " << *TrueWay << endl;
                return true;
            }
        };
        //cout << TagMemory[*SetOffsetPoint][*TrueWay] << endl;
        return false;
    }
    bool DirtyBitChecker(int way)
    {
                if (VirtualMemory[*SetOffsetPoint][way][0] == '1') //if dirty bit = 1 then true, otherwise false (almost the same as valid checker)
        {
            return true;
        } 
        else
        {
            return false;
        }
    }
    bool VCCase(string bincurrentaddress, bool prefetchdupe)
    {
        bool VCAccess = true;
        int VCHit = false;
        bool ValidDirty = false;
        for (int i = 0; i < Type; i++)
        {
            bool SetValidCheck = BlockValidityChecker(i);
            if (SetValidCheck == false)
            {   
                VCAccess = false;
                break;
            }
        };
        if (VCAccess == true)
        {   
            VCHit = VCPtr->Read(bincurrentaddress); //will call a swap if there's a hit in the victim cache
            if (VCHit == true)
            {
                SwapHitCounter ++;
                return true;
            }
        }
        return false;
        //LowerLevel->Read(bincurrentaddress); add back 
        /*int temp = VCPtr->LRUChecker();
        ValidDirty = VCPtr->DirtyBitChecker(temp); 
        VCPtr->Allocate(temp, ValidDirty, bincurrentaddress);*/
    };
    virtual void Allocate(int Temp, bool Dbit, string bincurrentaddress, bool prefetchdupe) //having the binary address as a parameter even after its decoded is so it can get redecoded
    //in the next write request to the next cache
    {   
        bool SwapHit = false;
        if (VCPtr != NULL) 
            SwapHit = VCCase(bincurrentaddress, prefetchdupe); 
            //cout <<SwapHit << endl;
        if (Dbit == true && VCPtr == NULL) //this is the dirty bit     
        {
            WriteBack(Temp, bincurrentaddress);
            WriteBacks ++;
            //TagMemory[*SetOffsetPoint][Temp] = *TagPoint; //TODO REMOVE IF ERROR;
        }
        if (LowerLevel != NULL && SwapHit == false)
            LowerLevel->Read(bincurrentaddress);
        //cout << TagMemory[*SetOffsetPoint][Temp] << endl;
        TagMemory[*SetOffsetPoint][Temp] = *TagPoint; //TODO POssibly need to update this so the correct tag gets placed //this is wrong during vcptr allocation we are updating the address with current
        //cout << TagMemory[*SetOffsetPoint][Temp] << endl;

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
    virtual void WriteBack (int LRUWay, string bincurrentaddress)
    {       
            string temp = DecToBinary(*SetOffsetPoint);
            while (temp.length() != LSBForSetIndex)
            {
                temp.insert(0, 1, '0');
            };
            string WriteBackAddress = TagMemory[*SetOffsetPoint][LRUWay] + temp; 
            //cout << WriteBackAddress << " " <<TagMemory[*SetOffsetPoint][LRUWay] << " " << temp;
            while (WriteBackAddress.length() != 32)
            {
                WriteBackAddress += '0';
            };
            //cout << WriteBackAddress << " ";
            if (LowerLevel != NULL)
                LowerLevel->Write(WriteBackAddress); //TODO Havent checked if this works at all 
    };  
    int LRUChecker () //might rename. This checks which way was least used for the write/read allocation
    {   

        /*for (int i = 0; i < VirtualMemory[0].size(); i++)   
            cout << VirtualMemory[0][i] << " ";
        cout << "Count" << endl;*/
        int max = 0; //I dont think the counter will need more digit than this 
        int Way = 0;
            if (Type != 1)
            {
                for (int i = 0; i < VirtualMemory[*SetOffsetPoint].size(); i++)
                {   
                    //cout << VirtualMemory[*SetOffsetPoint][i] << endl;
                    int temp = BinaryToDecimal(stoi(VirtualMemory[*SetOffsetPoint][i].substr(2, LRUCounterBits)));
                    //cout << temp << endl; 
                    if (temp > max)
                    {
                        max = temp;
                        Way = i;
                    }
                }
            }
            /*for (int i = 0; i < Type; i++)
            cout << VirtualMemory[*SetOffsetPoint][i] << " ";
            cout << "check" << endl; */
        return Way;
    };
    void LRUCounter () 
    {   
        if (Type != 1)
        {   
            /*for (int i = 0; i < Type; i++)
            cout << VirtualMemory[*SetOffsetPoint][i] << " ";
            cout << "Count" << endl; */
            int Bits = LRUCounterBits;
            string MostRecent = VirtualMemory[*SetOffsetPoint][*TrueWay];
            string Zeros;
            for (int i = 0; i < Bits; i++)
            {
                Zeros += "0";
            };
            VirtualMemory[*SetOffsetPoint][*TrueWay].replace(2, Bits, Zeros);
            for (int i = 0; i < Type; i++)
            {   
                //cout << VirtualMemory[*SetOffsetPoint][i].substr(2, LRUCounterBits) << endl;
                int temp = stoi(VirtualMemory[*SetOffsetPoint][i].substr(2, Bits));
                if(temp < stoi(MostRecent.substr(2, Bits)) && i != *TrueWay)
                {
                    temp = BinaryToDecimal(temp);
                    temp ++;
                    string str = DecToBinary(temp);
                    while (str.length() != Bits)
                    {
                        str = '0' + str;
                    }
                    VirtualMemory[*SetOffsetPoint][i].replace(2, Bits, str);
                }  
            }
        }
    };
    virtual void Swap()
    {   
        SwapRequests ++;
        //cout << "Swap Hit" << endl;
        int ConnectionTagBits = Connection->TagBits;
        int* ConnectionSet = Connection->SetOffsetPoint; 
        int* ConnectionWay = Connection->TrueWay;
        string TagTempC; //Victim Cache 
        string MemTempC; //Victim Cache
        string MemTempP; //main cache
        string TagTempP; //maincache
        string TagAdder = DecToBinary(*ConnectionSet); //used to add extra bits to the tag since main cache will have less tag bits than victim
        while (TagAdder.length() != Connection->LSBForSetIndex)
            TagAdder = '0' + TagAdder;
        TagTempC = Connection->TagMemory[*ConnectionSet][*ConnectionWay];
        TagTempC += TagAdder;
        MemTempC = Connection->VirtualMemory[*ConnectionSet][*ConnectionWay].substr(0, 2); 
        MemTempP = VirtualMemory[*SetOffsetPoint][*TrueWay].substr(0, 2);
        TagTempP = TagMemory[*SetOffsetPoint][*TrueWay];
        TagTempP.erase(ConnectionTagBits, TagTempP.length()); //delets extra charachters from tag because victim cache is fully associative and thus has a longer tag
        VirtualMemory[0][*TrueWay].replace(0, 2, MemTempC);
        TagMemory[0][*TrueWay] = TagTempC;
        Connection->VirtualMemory[*ConnectionSet][*ConnectionWay].replace(0, 2 , MemTempP);
        Connection->TagMemory[*ConnectionSet][*ConnectionWay] = TagTempP; //TODO Have not checked if this will work at all
    };
    virtual void Reset(vector<string> pushback) //this isnt pure virtual because the class becomes abstract and cant create any objects with it
    {
    };
};
class Victim : public Cache
{
public: 
    Victim(Cache * connection, int blocks, string name, int size, int type, int blocksize, Cache * lowerlevel):Cache(connection, name, size, type, blocksize, lowerlevel)
    {
        Blocks = blocks;
        Type = Blocks;
        BlockSize = Connection->BlockSize;
    };
    bool Read (string bincurrentaddress)
    {   
        while (bincurrentaddress.length() != 32)
        {
            bincurrentaddress.insert(0, 1,'0');
        }
        AddressDecoder(bincurrentaddress);
        bool ValidTag = TagChecker(); //false if no tag matches the address tag, true otherwise
        bool ValidBit = BlockValidityChecker(*TrueWay);
        bool ValidDirty = DirtyBitChecker(*TrueWay);
        if (ValidBit == true && ValidTag == true)
        {       
                bool* PrefetchDupe = Connection->PrefetchDupe;
                Hits ++; //= data; since I'm only doing blocks with no data IM not implementing the data rn
                Reads ++;
                if (*PrefetchDupe == false)
                {
                    Swap();       
                    *SwapHitCheck = true;
                    LRUCounter();
                    return true;
                }
                else
                {
                    int LRUWay = LRUChecker(); 
                    Misses ++;
                    ReadMiss ++;
                    Reads ++;
                    ValidDirty = DirtyBitChecker(LRUWay);
                    *TrueWay = LRUWay;
                    Allocate(LRUWay, ValidDirty, bincurrentaddress);
                    LRUCounter();
                    *SwapHitCheck = false;
                    return false;
                }
        }
        else
        {   //this is the read miss
            int LRUWay = LRUChecker(); 
            Misses ++;
            ReadMiss ++;
            Reads ++;
            ValidDirty = DirtyBitChecker(LRUWay);
            *TrueWay = LRUWay;
            Allocate(LRUWay, ValidDirty, bincurrentaddress);
            LRUCounter();
            *SwapHitCheck = false;
            return false;
        }
    };
    void Allocate(int Temp, bool Dbit, string bincurrentaddress)
    {   
        if (Dbit == true)
            WriteBack(Temp, bincurrentaddress); 
        int ConnectionTagBits = Connection->TagBits;
        int* ConnectionSet = Connection->SetOffsetPoint;
        int* ConnectionWay = Connection->TrueWay;
        string TagTempC; //Victim Cache 
        string MemTempC; //Victim Cache
        string TagAdder = DecToBinary(*ConnectionSet);
        while (TagAdder.length() != Connection->LSBForSetIndex)
            TagAdder = '0' + TagAdder;
        TagTempC = Connection->TagMemory[*ConnectionSet][*ConnectionWay];
        TagTempC += TagAdder;
        MemTempC = Connection->VirtualMemory[*ConnectionSet][*ConnectionWay].substr(0,2); 
        TagMemory[0][Temp] = TagTempC;
        VirtualMemory[0][Temp].replace(0, 2, MemTempC);
        return;
    }
    void WriteBack (int LRUWay, string bincurrentaddress)
    {       
            string WriteBackAddress = TagMemory[*SetOffsetPoint][LRUWay];
            //WriteBackAddress += VirtualMemory[*SetOffsetPoint][LRUWay].substr(2, Connection->LSBForSetIndex); //Possible fix it wasnt :(
            //cout << WriteBackAddress << " " <<TagMemory[*SetOffsetPoint][LRUWay] << " " << temp;

            while (WriteBackAddress.length() != 32)
            {
                WriteBackAddress += '0';
            };
            //cout << WriteBackAddress << " ";
            WriteBacks ++;
            Connection->LowerLevel->Write(WriteBackAddress); //TODO Havent checked if this works at all 
    };  
    void Reset (vector<string> pushback) override {}; 
};

class PrefetchBuffer : public Cache
{
public: 
    vector<string> BAMemory; //Buffer address memory
    bool PrefetchMethod = false; //flase is tagges sequential while true is markov predictor 
    int AddressSize;
    PrefetchBuffer(Cache* connection, string name, int size, int type, int blocksize, Cache * lowerlevel):Cache(connection, name, size, type, blocksize, lowerlevel)
    {
        Initialize();
    };
    void Initialize ()
    {
        int m = 16; //this is dumb but I dont feel like adding it to the main constructor so edit buffer size here this is in total blocks
        AddressSize = (32 - BlockOffsetBits);

        BAMemory.resize(m); //first layer of virtual memory is set
        for (int i = 0; i < BAMemory.size(); i++)
        {
            while (BAMemory[i].size() != AddressSize)
            {
                BAMemory[i] += '0';
            }
        };
    }
    bool Read (string bincurrentaddress) //sees if the current address matches the block in the front of the buffer 
    {   
        GHBEntry* NewEntry = GHBPtr->Insert(bincurrentaddress, BlockOffsetBits);
        IndexTable->LookUp(NewEntry); 
        vector<string> temp = GHBPtr->MarkovPredictor(NewEntry);
        /*for (int i = 0; i < temp.size(); i ++)  
        {
            cout << temp[i] << endl;
        } 
        cout << "End" << endl; */ 
        PrefetchAllocate(temp); //clears and replaces the data in the buffer for the most recent miss address markov predictor
        for (int i = 0; i < BAMemory.size(); i++)
        {
            cout << BAMemory[i] << endl;
            cout << bincurrentaddress << "bincurr" << endl;
            if (BAMemory[i].substr(0, AddressSize) == bincurrentaddress.substr(0, AddressSize))
            {
                swap(BAMemory[0], BAMemory[i]); //swaps the front of the stream buffer with the correct address after associative search
                PrefetchSaves ++;
                return true;
            }
        }
        cout << "End" << endl;
        return false;
    };
    void PrefetchAllocate (vector<string> pushback) //sets the BAMemory and checks for duplicates
    {   
        int Index1 = 0;
        for (string& element1 : pushback)
        {
            for (string& element2 : BAMemory) //cool iterator based on the reference to the original vector
            {
                if (element1 == element2)
                {
                    pushback[Index1] = "0";
                }
            }
        Index1 ++;
        }
        vector<string>::iterator EraseIterator = BAMemory.end() - pushback.size(); //can create iterator that goes to index before pushback
        BAMemory.erase(EraseIterator, BAMemory.end());  
        BAMemory.insert(BAMemory.begin(), pushback.begin(), pushback.end());

        /*for (int i = 0; i < BAMemory.size(); i++)
        {
            cout << BAMemory[i] << endl; 
        };
        cout << "END" << endl;*/ 
    };
};

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