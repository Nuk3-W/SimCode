#ifndef GHB_H
#define GHB_H

#include "function.h"

class GHB;

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
        missaddress = missaddress.substr(0, 32 - blockbits);
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
        MarkovTable.resize(BufferSize); //TODO check if works
        GHBEntry* Current = missreference->Duplicate; 
        vector<string> Max; //this is to keep track of the most entries after the miss address
        Max.reserve(16);//can go up to 16 but im using the size to make an iterator so keep the actual vector smaller than 16 unless necessary
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
        for (int i = 0; i < Max.capacity(); i++)  //iterate 16 times to get the highest probable next addresses based on markov predictor
        { 
            //cout << MarkovTable[i].Address << endl;    
            if (MarkovTable[i].Address != "0")
                Max.push_back(MarkovTable[i].Address);
        };
        return Max;
    };
};

#endif