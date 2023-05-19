#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "matrix_format.h"
#include "spm_allocated.h"
#include "mmio.h"
#include "mmio.c"
#ifndef NATIVE
#include "gem5/m5ops.h"
#endif
#include <arm_sve.h>

#ifdef SP
typedef float FLOAT;
#else
typedef double FLOAT;
#endif

int WARMUP_NUM = 0;
int EXECUTE_NUM = 1;

void SpMV(CSRMatrix &csr, FLOAT *x, FLOAT *y)
{
	int row = csr.row;
	for (int i = 0; i < row; i++)
	{
		y[i] = 0;
		for (int j = (csr.row_ptr[i]); j < (csr.row_ptr[i + 1]); j++)
		{
			y[i] += csr.values[j] * x[csr.col_idx[j]];
		}
	}
}

void print_svfloat32(svbool_t pg, svfloat32_t vec)
{
	int vec_size = svcntw();
	float buffer[vec_size];

	svst1(pg, buffer, vec);

	printf("[ ");

	for (int i = 0; i < vec_size; i++)
	{
		if (i < vec_size - 1)
			printf("%f, ", buffer[i]);
		else
			printf("%f", buffer[i]);
	}

	printf("]\n");
}

void print_svint32(svbool_t pg, svint32_t vec)
{
	int vec_size = svcntw();
	int32_t buffer[vec_size];

	svst1(pg, buffer, vec);

	printf("[ ");

	for (int i = 0; i < vec_size; i++)
	{
		if (i < vec_size - 1)
			printf("%f, ", buffer[i]);
		else
			printf("%f", buffer[i]);
	}

	printf("]\n");

}

void print_svfloat64(svbool_t pg, svfloat64_t vec)
{
	int vec_size = svcntd();
	double buffer[vec_size];

	svst1(pg, buffer, vec);

	printf("[ ");

	for (int i = 0; i < vec_size; i++)
	{
		if (i < vec_size - 1)
			printf("%f, ", buffer[i]);
		else
			printf("%f", buffer[i]);
	}

	printf("]\n");

}

void print_svuint64(svbool_t pg, svuint64_t vec)
{
	int vec_size = svcntd();
	uint64_t buffer[vec_size];

	svst1(pg, buffer, vec);

	printf("[ ");

	for (int i = 0; i < vec_size; i++)
	{
		if (i < vec_size - 1)
			printf("%llu, ", (unsigned long long)buffer[i]);
		else
			printf("%lld", (unsigned long long)buffer[i]);
	}

	printf("]\n");

}

// void SpMV_neon(FLOAT *y, CSR *a, FLOAT *x)
void SpMV_sve(CSRMatrix &csr, FLOAT *x, FLOAT *y)
{
    int nrows = csr.row;

	int vec_size1 = svcntw();
	int vec_size2 = svcntd();

// #pragma omp parallel for
    for(int i = 0; i < nrows; ++i)
    {
        svfloat32_t svsum = svdup_f32(0.0);
		svfloat64_t svsum1 = svdup_f64(0.0);

		int start = csr.row_ptr[i];
		int end = csr.row_ptr[i + 1];

		svfloat32_t svxv;
		svfloat64_t svxv1;
		
#ifdef SP
        for(int j = start; j < end; j += vec_size1)
        {
			int cnt = std::min(end - j, vec_size1);
            svbool_t pg = svwhilelt_b32(0, cnt);

            svfloat32_t values = svld1_f32(pg, &csr.values[j]);
			// print_svfloat32(pg, values);
			// printf("---------- matrix values ---------------\n");

			svint32_t indices = svld1_s32(pg, &csr.col_idx[j]);
			// print_svint32(pg, indices);
			// printf("--------------------- matrix index ---------\n");

			svxv = svld1_gather_s32index_f32(pg, x, indices);
			// print_svfloat32(pg, svxv);
			// printf("----------------- vector value ------------\n");

			svsum = svmla_f32_m(pg, svsum, values, svxv);
        }
		y[i] = svaddv(svptrue_b32(), svsum);
        
#else	
		
		for(int j = start; j < end; j += vec_size2)
        {
			int cnt = std::min(end - j, vec_size2);
            svbool_t pg = svwhilelt_b64(0, cnt);

            svfloat64_t values = svld1_f64(pg, &csr.values[j]);
			// print_svfloat64(pg, values);
            // printf("---------------------------- matrix values ----------------------------\n");
			
			svuint64_t indices = svld1sw_u64(pg, &csr.col_idx[j]);
			// print_svuint64(pg, indices);
            // printf("----------------------------------- index ------------------------\n");
			
			svxv1 = svld1_gather_u64index_f64(pg, x, indices);
			// print_svfloat64(pg, svxv1);
            // printf("---------------------------- vector values ----------------------------\n");

			svsum1 = svmla_f64_m(pg, svsum1, values, svxv1);
			// print_svfloat64(pg, svsum1);
            // printf("----------------------- sum vector -------------------\n");

        }
		// printf("---------------------------------------------------\n");
		y[i] = svaddv(svptrue_b64(), svsum1);
        
#endif
        
    }
}

void execute_SpMV(CSRMatrix &csr, int position, int mode)
{

	int col = csr.col;
	FLOAT *x;
	// FLOAT y[col] = {0};
	FLOAT y[col];

	if (mode == 0)
		printf("Execution with scalar\n");
	else if (mode == 1)
		printf("Execution with sve\n");

	if (position == 0) // allocate in SPM
	{
		printf("Executing SpMV with SPM...\n");
		uint64_t offset = 6442450944;
		x = (FLOAT *)spm_mem_alloc(sizeof(FLOAT) * col, offset);
	}
	else if (position == 1) // allocate in memory
	{
		printf("Executing SpMV without SPM...\n");
		x = (FLOAT *)malloc(sizeof(FLOAT) * col);
	}
	else
	{
		printf("Invalid position: %d\n", position);
		exit(1);
	}

	if (x == NULL)
	{
		printf("Allocate vector x failed.\n");
		exit(1);
	}

	for (int i = 0; i < col; i++)
		x[i] = 1.0;

	for (int i = 0; i < WARMUP_NUM + EXECUTE_NUM; i++)
	{
		#ifndef NATIVE
		if (i == WARMUP_NUM)
			m5_reset_stats(0, 0);
		#endif
		if (mode == 0)
			SpMV(csr, x, y);
		else if (mode == 1)
			SpMV_sve(csr, x, y);
	}
	#ifndef NATIVE
	m5_dump_stats(0, 0);
	#endif
	printf("SpMV execution finish.\n\n");

	for (int i = 0; i < csr.col; i++)
	{
#ifdef SP
		printf("y[%d] = %f\n", i, y[i]);
#else
		printf("y[%d] = %lf\n", i, y[i]);
#endif
		
	}

	// free(x);
	// free(y);
}

int main(int argc, char *argv[])
{

	int position; // 0: SPM     1: Memory
	int mode;     // 0: scalar  1: sve

	if (argc != 6)
	{
		printf("Missing parameters.\n");
		exit(1);
	}

	position = atoi(argv[1]);
	mode = atoi(argv[2]);

	int ret_code;
    MM_typecode matcode;
    FILE *f;
    int M, N, nz;   
    int i, *I, *J;
#ifdef SP
	FLOAT *val;
#else
	FLOAT *val;
#endif
	
    if (argc < 6)
	{
		fprintf(stderr, "Usage: %s [martix-market-filename]\n", argv[3]);
		exit(1);
	}
    else    
    { 
        if ((f = fopen(argv[3], "r")) == NULL) 
            exit(1);
    }

    if (mm_read_banner(f, &matcode) != 0)
    {
        printf("Could not process Matrix Market banner.\n");
        exit(1);
    }


    /*  This is how one can screen matrix types if their application */
    /*  only supports a subset of the Matrix Market data types.      */

    if (mm_is_complex(matcode) && mm_is_matrix(matcode) && 
            mm_is_sparse(matcode) )
    {
        printf("Sorry, this application does not support ");
        printf("Market Market type: [%s]\n", mm_typecode_to_str(matcode));
        exit(1);
    }

	if(mm_is_array(matcode))
	{
		printf("do not support the array matrix!\n");
		exit(1);
	}

    /* find out size of sparse matrix .... */

    if ((ret_code = mm_read_mtx_crd_size(f, &M, &N, &nz)) !=0)
        exit(1);

    /* reseve memory for matrices */

    I = (int *) malloc(nz * sizeof(int));
    J = (int *) malloc(nz * sizeof(int));
    val = (FLOAT *) malloc(nz * sizeof(FLOAT));


    /* NOTE: when reading in doubles, ANSI C requires the use of the "l"  */
    /*   specifier as in "%lg", "%lf", "%le", otherwise errors will occur */
    /*  (ANSI C X3.159-1989, Sec. 4.9.6.2, p. 136 lines 13-15)            */

    if(mm_is_pattern(matcode))
    {
        for (i=0; i<nz; i++)
        {
            fscanf(f, "%d %d\n", &J[i], &I[i]);
            // printf("%d %d\n", I[i], J[i]);
            I[i]--;  /* adjust from 1-based to 0-based */
            J[i]--;
            val[i] = 1.0;
// #ifdef SP
//             printf("%f\n", val[i]);
// #else 
// 			printf("%lf\n", val[i]);
// #endif
        }
    }
    else if(mm_is_real(matcode) || mm_is_integer(matcode))
    {
        for(i=0; i<nz; i++)
        {
#ifdef SP
			fscanf(f, "%d %d %f\n", &J[i], &I[i], &val[i]);
#else
            fscanf(f, "%d %d %lf\n", &J[i], &I[i], &val[i]);
#endif
//             printf("%d %d\n", I[i], J[i]);
// #ifdef SP
// 			printf("%f\n", val[i]);
// #else
// 			printf("%lf\n", val[i]);
// #endif
            I[i]--;  /* adjust from 1-based to 0-based */
            J[i]--;
        }
    }
    

    if (f !=stdin) fclose(f);

    /************************/
    /* now write out matrix */
    /************************/
    CSRMatrix csr;
    COO_To_CSR(M, N, nz, I, J, val, csr);

    mm_write_banner(stdout, matcode);
    mm_write_mtx_crd_size(stdout, M, N, nz);
    
	WARMUP_NUM = atoi(argv[4]);
	EXECUTE_NUM = atoi(argv[5]);

	// Print_CSR(csr);

	printf("matrix: %s\n", argv[3]);
	printf("rows: %d\tcols: %d\tnnz: %d\n", csr.row, csr.col, csr.nnz);
	printf("WARMUP_NUM: %d\tEXECUTE_NUM: %d\n", WARMUP_NUM, EXECUTE_NUM);

	switch (position)
	{
	case 0:
		execute_SpMV(csr, 0, mode);
		break;
	case 1:
		execute_SpMV(csr, 1, mode);
		break;
	case 2:
		execute_SpMV(csr, 0, mode);
		execute_SpMV(csr, 1, mode);
		break;
	default:
		break;
	}

	return 0;
}
