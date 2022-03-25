// multi-thread
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <iostream>
#include <fstream>

#include <sstream>
#include <typeinfo>

#include <vector>
#include <utility>
#include <functional>
#include <algorithm>
#include <bitset>
#include <time.h>
#include <pthread.h>

using namespace std;

#define LINE_BUF_SIZE 256
#define NUM_THREADS 7

char linebuf[LINE_BUF_SIZE];
char *word;
char *tmp;

struct CSVDATA
{
    vector<vector<string> > data;
    int record_count;
    int col_count;
    char *name;
};
CSVDATA csvData_1, csvData_1_hash;
CSVDATA csvData_2, csvData_2_hash;
vector<string> hash_hll_1;
vector<string> hash_hll_2;
vector<int> record;

// prototype
pair<CSVDATA, CSVDATA>
get_record_col_count(const char *path, CSVDATA csvdata, CSVDATA csvdata_hash);
pair<CSVDATA, CSVDATA> insert_structure(CSVDATA csvdata, CSVDATA csvdata_hash, int record_count, int col_count, FILE *fp, const char *path);
void print_csv(CSVDATA csvdata);
string StrToBitStr(string str);
pair<vector<string>, vector<string> > HLL(CSVDATA csvData_1, CSVDATA csvData_1_hash, CSVDATA csvData_2, CSVDATA csvData_2_hash);
//

// read csv, get the size of columns and records
pair<CSVDATA, CSVDATA> get_record_col_count(const char *path, CSVDATA csvdata, CSVDATA csvdata_hash)
{
    int record_count = 0;
    int col_count = 0;
    FILE *fp = fopen(path, "r");

    while (fgets(linebuf, LINE_BUF_SIZE, fp))
    {
        col_count = 0;

        word = strtok_r(linebuf, ",", &tmp);

        while (word != NULL)
        {
            strtol(word, NULL, 0);
            col_count++;
            word = strtok_r(NULL, ",", &tmp);
        }

        record_count++;
    }

    csvdata.record_count = record_count;
    csvdata.col_count = col_count;
    csvdata_hash.record_count = record_count;
    csvdata_hash.col_count = col_count;

    csvdata.data.resize(record_count);
    for (int i = 0; i < record_count; i++)
    {
        csvdata.data[i].resize(col_count);
    }

    csvdata_hash.data.resize(record_count);
    for (int i = 0; i < record_count; i++)
    {
        csvdata_hash.data[i].resize(col_count);
    }
    fclose(fp);

    csvdata = insert_structure(csvdata, csvdata_hash, csvdata.record_count, csvdata.col_count, fp, path).first;
    csvdata_hash = insert_structure(csvdata, csvdata_hash, csvdata.record_count, csvdata.col_count, fp, path).second;
    return make_pair(csvdata, csvdata_hash);
}

// insert csv data into struct
pair<CSVDATA, CSVDATA> insert_structure(CSVDATA csvdata, CSVDATA csvdata_hash, int record_count, int col_count, FILE *fp, const char *path)
{

    string word_hash;

    record_count = 0;
    fp = fopen(path, "r");
    while (fgets(linebuf, LINE_BUF_SIZE, fp))
    {
        col_count = 0;

        word = strtok_r(linebuf, ",", &tmp);
        // cout << word << endl;
        hash<string> hash_obj;

        while (word != NULL)
        {
            csvdata.data[record_count][col_count] = word;
            stringstream hash_data;
            hash_data << hash_obj(word);
            word_hash = hash_data.str();
            csvdata_hash.data[record_count][col_count] = word_hash;
            col_count++;
            word = strtok_r(NULL, ",", &tmp);
        }

        record_count++;
    }
    fclose(fp);
    return make_pair(csvdata, csvdata_hash);
}

void print_csv(CSVDATA csvdata)
{

    for (int i = 0; i < csvdata.record_count; i++)
    {
        for (int c = 0; c < csvdata.col_count; c++)
        {
            cout << csvdata.data[i][c] << endl;
        }
    }
}

// hll, inverted index, generate ind
pair<vector<string>, vector<string> > HLL(CSVDATA csvData_1, CSVDATA csvData_1_hash, CSVDATA csvData_2, CSVDATA csvData_2_hash)
{
    // HLL for csv1 and csv2, hashvalue->binary->first four bits
    int i = 0;
    while (i < csvData_1_hash.record_count)
    {
        string hll = "";
        for (int c = 0; c < csvData_1_hash.col_count; c++)
        {
            hll = hll + StrToBitStr(csvData_1_hash.data[i][c]);
        }
        hash_hll_1.push_back(hll);
        i++;
    }

    int m = 0;
    while (m < csvData_2_hash.record_count)
    {
        string hll = "";
        for (int c = 0; c < csvData_2_hash.col_count; c++)
        {
            hll = hll + StrToBitStr(csvData_2_hash.data[m][c]);
        }
        hash_hll_2.push_back(hll);
        m++;
    }
    return make_pair(hash_hll_1, hash_hll_2);
}

// get binary HLL
string StrToBitStr(string str)
{
    str = str.substr(0, 19);
    long binary;
    binary = atol(str.c_str());

    string r;
    while (binary != 0)
    {
        r += (binary % 2 == 0 ? "0" : "1");
        binary /= 2;
    }
    reverse(begin(r), end(r));
    // cout << r << endl;
    string s = r.substr(0, 4);
    return s;
}

void *IND_check(void *arg)
{
    // compare HLL binary string by tuple, check each value (= inverted index)
    int start = (long)arg * 1000;
    cout << start << endl;
    for (int i = start; i < start + 1000; i++)
    {
        for (int j = 0; j < hash_hll_2.size(); j++)
        {
            if (hash_hll_1[i] == hash_hll_2[j])
            {
                if (csvData_1_hash.data[i] == csvData_2_hash.data[j])
                {
                    record.push_back(i);
                }
            }
        }
    }
    // cout << 1 << endl;
    // for (int i = 0; i < record.size(); i++)
    // {
    //     cout << record[i] << endl;
    // }
    sort(record.begin(), record.end());
    record.erase(unique(record.begin(), record.end()), record.end());
    ofstream outFile;
    outFile.open("data555.csv", ios::out);
    for (int i = 0; i < record.size(); i++)
    {
        for (int j = 0; j < csvData_1.col_count; j++)
        {
            outFile << csvData_1.data[record[i]][j] << endl;
        }
    }
    outFile.close();
    return NULL;
    // result output
}

// void output(vector<int> record, CSVDATA csvData_1)
// {
//     sort(record.begin(), record.end());
//     record.erase(unique(record.begin(), record.end()), record.end());
//     ofstream outFile;
//     outFile.open("data555.csv", ios::out);
//     for (int i = 0; i < record.size(); i++)
//     {
//         for (int j = 0; j < csvData_1.col_count; j++)
//         {
//             outFile << csvData_1.data[record[i]][j] << endl;
//         }
//     }
//     outFile.close();
// }
// void thread_hll()
// {

// }

int main(void)
{
    clock_t begin = clock();
    string filepath1_1 = "csv/case1.csv";
    string filepath2_1 = "csv/case2.csv";
    const char *filepath1 = filepath1_1.c_str();
    const char *filepath2 = filepath2_1.c_str();
    csvData_1 = get_record_col_count(filepath1, csvData_1, csvData_1_hash).first;
    csvData_1_hash = get_record_col_count(filepath1, csvData_1, csvData_1_hash).second;
    csvData_2 = get_record_col_count(filepath2, csvData_2, csvData_2_hash).first;
    csvData_2_hash = get_record_col_count(filepath2, csvData_2, csvData_2_hash).second;
    // print_csv(csvData_1);
    // print_csv(csvData_1_hash);
    // print_csv(csvData_2);
    // print_csv(csvData_2_hash);
    hash_hll_1 = HLL(csvData_1, csvData_1_hash, csvData_2, csvData_2_hash).first;
    hash_hll_2 = HLL(csvData_1, csvData_1_hash, csvData_2, csvData_2_hash).second;
    // IND_check(hash_hll_1, hash_hll_2, csvData_1_hash, csvData_2_hash, csvData_1);
    // thread_hll();
    pthread_t threads[NUM_THREADS];

    for (int m = 0; m < NUM_THREADS; m++)
    {

        int rc;
        rc = pthread_create(&threads[m], NULL, IND_check, (void *)m);
        // if (rc)
        // {
        //     cout << "Error:unable to create thread," << rc << endl;
        //     exit(-1);
        // }
    }

    pthread_exit(NULL);

    // merge(record1.begin(), record1.end(), record2.begin(), record2.end(), record4.begin());
    // merge(record4.begin(), record4.end(), record3.begin(), record3.end(), record5.begin());
    // output(record, csvData_1);
    clock_t end = clock();
    cout << "Running time: " << (double)(end - begin) / CLOCKS_PER_SEC * 1000 << "ms" << endl;
}

/*
目前最佳版本
可读csv行列数
*/