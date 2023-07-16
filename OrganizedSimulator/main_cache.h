#ifndef Cache_H
#define Cache_H
#include "GHB.h"
#include "function.h"
#include "control_unit.h"

class GHB;
class IT;

using namespace std;

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
                VirtualMemory[i][j] += "0"; //this is the tag bit for the tagged seqeuntial prefetcher
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
            *PrefetchDupe = false; //TODO add a call to the control unit to choose the prefetcher
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
            Allocate(LRUWay, ValidDirty, bincurrentaddress, *PrefetchDupe); 
            
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
            if (VCPtr != NULL)
                VirtualMemory[*SetOffsetPoint][LRUWay].replace((VirtualMemory[*SetOffsetPoint][LRUWay].length() - 1), 1, "1"); //sets the tag bit to 1
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
            Allocate(LRUWay, ValidDirty, bincurrentaddress, *PrefetchDupe); //NOT IMPLEMENTED YET
            LRUCounter();
            VirtualMemory[*SetOffsetPoint][LRUWay][1] = '1'; //sets valid bit to true
            VirtualMemory[*SetOffsetPoint][LRUWay][0] = '1'; //sets dirty bit to true
            VirtualMemory[*SetOffsetPoint][LRUWay].replace((VirtualMemory[*SetOffsetPoint][LRUWay].length() - 1), 1, "1");
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
    bool VCCase(string bincurrentaddress)
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
    virtual void Allocate(int Temp, bool Dbit, string bincurrentaddress, bool &prefetchdupe) //having the binary address as a parameter even after its decoded is so it can get redecoded
    //in the next write request to the next cache 
    {   
        bool SwapHit = false;
        if (VCPtr != NULL) 
            SwapHit = VCCase(bincurrentaddress); 
            //cout <<SwapHit << endl;
        if (Dbit == true && VCPtr == NULL) //this is the dirty bit     
        {
            WriteBack(Temp, bincurrentaddress);
            WriteBacks ++;
            //TagMemory[*SetOffsetPoint][Temp] = *TagPoint; 
        }
        if (LowerLevel != NULL && SwapHit == false  && prefetchdupe == false)
            LowerLevel->Read(bincurrentaddress);
        //cout << TagMemory[*SetOffsetPoint][Temp] << endl;
        TagMemory[*SetOffsetPoint][Temp] = *TagPoint; 

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
                LowerLevel->Write(WriteBackAddress); 
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
        VirtualMemory[0][*TrueWay].replace(VirtualMemory[0][*TrueWay].length() - 1, 1, "0"); //TODO Check if works 
        TagMemory[0][*TrueWay] = TagTempC;
        Connection->VirtualMemory[*ConnectionSet][*ConnectionWay].replace(0, 2 , MemTempP);
        Connection->TagMemory[*ConnectionSet][*ConnectionWay] = TagTempP;
    };
    virtual void Reset(vector<string> pushback) //this isnt pure virtual because the class becomes abstract and cant create any objects with it
    {
    };
};
class Victim : public Cache
{
public: 
    bool* PrefetchDupe = Connection->PrefetchDupe;
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
        PrefetchDupe = Connection->PrefetchDupe;
        if (ValidBit == true && ValidTag == true)
        {       
                if (*PrefetchDupe == false)
                {   
                    Hits ++; //= data; since I'm only doing blocks with no data IM not implementing the data rn
                    Reads ++;
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
            Connection->LowerLevel->Write(WriteBackAddress);
    };  
    void Reset (vector<string> pushback) override {}; 
};



class PrefetchBuffer : public Cache
{
public: 
    vector<string> BAMemory; //Buffer address memory
    bool PrefetchMethod = false; //flase is tagges sequential while true is markov predictor 
    int AddressSize;
    //ControlUnit* CU;

    PrefetchBuffer(Cache* connection, string name, int size, int type, int blocksize, Cache * lowerlevel):Cache(connection, name, size, type, blocksize, lowerlevel)
    {
        Initialize();
    };
    void Initialize ()
    {
        int m = 16; //this is dumb but I dont feel like adding it to the main constructor so edit buffer size here this is in total blocks
        AddressSize = (32 - BlockOffsetBits);
        BAMemory.resize(m, "0"); //first layer of virtual memory is set
    }
    /*void SetCU (ControlUnit* cu)
    {
        CU = cu;
    }; */
    bool Read (string bincurrentaddress) //sees if the current address matches the block in the front of the buffer 
    {   
        GHBEntry* NewEntry = GHBPtr->Insert(bincurrentaddress, BlockOffsetBits);
        IndexTable->LookUp(NewEntry); 
        /*if ()
        {

        } */
        vector<string> temp = GHBPtr->MarkovPredictor(NewEntry);
        /*for (int i = 0; i < temp.size(); i ++)  
        {
            cout << temp[i] << endl;
        } */
        //cout << "End" << endl; 
        PrefetchAllocate(temp); //clears and replaces the data in the buffer for the most recent miss address markov predictor
        for (int i = 0; i < BAMemory.size(); i++)
        {
            //cout << BAMemory[i] << endl;
            //cout << bincurrentaddress << "bincurr" << endl;
            if (BAMemory[i].substr(0, AddressSize) == bincurrentaddress.substr(0, AddressSize))
            {
                swap(BAMemory[0], BAMemory[i]); //swaps the front of the stream buffer with the correct address after associative search
                PrefetchSaves ++;
                return true;
            }
        }
        //cout << "End" << endl;
        return false;
    };
    void PrefetchAllocate (vector<string> pushback) //sets the BAMemory and checks for duplicates
    {   
        if (BAMemory[0] == "0")
        {
            int Index = 1; 
            while (BAMemory[Index] != "0")
            {
                if (Index == 16)
                    break;
                Index ++;
            }
            swap(BAMemory[0], BAMemory[Index]); 
        }
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
        //cout << pushback.size() << endl;
        int temp = pushback.size();
        pushback.erase(std::remove(pushback.begin(), pushback.end(), "0"), pushback.end());
        /*for (int i = 0; i < pushback.size(); i++)
        {   
            cout << pushback[i] << endl;
        }; */

        //cout << pushback.size() << " pushback size" << endl; 
        vector<string>::iterator EraseIterator = BAMemory.end(); //can create iterator that goes to index before pushback
        advance(EraseIterator, -(pushback.size())); 
        /*for (int i = 0; i < BAMemory.size(); i++)
        {   
            cout << BAMemory[i] << endl;
        };*/ 
        //cout << pushback.size() << " pushback size" << endl;
        BAMemory.erase(EraseIterator, BAMemory.end());  
        /*for (int i = 0; i < BAMemory.size(); i++)
        {   
            cout << BAMemory[i] << endl;
        };
        cout << "After Erase" << endl; */
        BAMemory.insert(BAMemory.begin(), pushback.begin(), pushback.end());
        /*for (int i = 0; i < BAMemory.size(); i++)
        {
            cout << BAMemory[i] << endl;
            if (BAMemory[i] == "0")
                cout << "False Entry" << endl;
        }; 
        cout << "After Insert" << endl; */
        //cout << "END" << endl;
        /*for (int i = 0; i < BAMemory.size(); i++)
        {
            cout << BAMemory[i] << endl; 
        };
        cout << "END" << endl;*/ 
    };
};

#endif