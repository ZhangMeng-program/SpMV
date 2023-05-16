#include <vector>
#include <iostream>
#include <stdio.h>

#ifdef SP
typedef float FLOAT;
#else
typedef double FLOAT;
#endif

using std::vector;
using std::cout;
using std::endl;

struct CSRMatrix
{
    vector<int> row_ptr;
    vector<int> col_idx;
#ifdef SP
    vector<FLOAT> values;
#else
    vector<FLOAT> values;
#endif
    int nnz;
    int row;
    int col;
};

void COO_To_CSR(const int M, const int N, const int NNZ, 
                int *coo_row, int *coo_col, FLOAT *val, CSRMatrix &csr)
{
    csr.row = M;
    csr.col = N;
    csr.nnz = NNZ;

    csr.values.resize(NNZ);
    csr.col_idx.resize(NNZ);
    csr.row_ptr.resize(M + 1);

//生成CSR row_ptr数组

    for(int i = 0; i < NNZ; i++)
    {
       csr.row_ptr[coo_row[i] +1]++;
    }

    for (int i = 1; i < csr.row_ptr.size(); i++)
    {
        csr.row_ptr[i] += csr.row_ptr[i - 1];
    }

    for (int i = 0; i < NNZ; i++)
    {
        csr.col_idx[i] = coo_col[i];
        csr.values[i] = val[i];
    }
}

void Print_CSR(CSRMatrix &csr)
{
    cout << "Matrix Row: " << csr.row << endl;
    cout << "Matrix Col: " << csr.col << endl;
    cout << "Matrix NNZ: " << csr.nnz << endl;

    int nnz = csr.nnz;
    int size = csr.row + 1;
    
    cout << "------------------- Row Ptr------------------" << endl;
    for (int i = 0; i < size; i++)
    {
        cout << csr.row_ptr[i] << endl;
    }

    cout << "------------------- Col Idx------------------" << endl;
    for (int i = 0; i < nnz; i++)
    {
        cout << csr.col_idx[i] << endl;
    }

    cout << "------------------- Matrix Val---------------------" << endl;
    for (int i = 0; i < nnz; i++)
    {
#ifdef SP
        printf("%f\n", csr.values[i]);
#else 
        printf("%lf\n", csr.values[i]);
#endif
    }
}
