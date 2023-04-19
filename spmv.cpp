#ifdef SP
typedef float FLOAT;
#else
typedef double FLOAT;
#endif

#include "csr_formatter.h"
#include "spm_allocated.h"
#ifndef NATIVE
#include "gem5/m5ops.h"
#endif
#include <arm_neon.h>

int WARMUP_NUM = 0;
int EXECUTE_NUM = 1;

void SpMV(CSR &csr, FLOAT *x, FLOAT *y)
{
	for (int i = 0; i < csr.row_ptr.size() - 1; i++)
	{
		y[i] = 0;
		for (int j = (csr.row_ptr[i] - 1); j < (csr.row_ptr[i + 1] - 1); j++)
		{
			y[i] += csr.val[j] * x[csr.col_ind[j] - 1];
		}
	}
}


// void SpMV_neon(FLOAT *y, CSR *a, FLOAT *x)
void SpMV_neon(CSR &csr, FLOAT *x, FLOAT *y)
{
    int nrows = csr.row;

// #pragma omp parallel for
    for(int i = 0; i < nrows; ++i)
    {
        FLOAT sum = 0;

        int idx;
#ifdef SP
        for(idx = csr.row_ptr[i]; idx <= csr.row_ptr[i+1] - 4; idx += 4)
        {
            float32x4_t vsum, va, vx;
            FLOAT xtmp[4];

            xtmp[0] = x[csr.col_ind[idx]];
            xtmp[1] = x[csr.col_ind[idx+1]];
            xtmp[2] = x[csr.col_ind[idx+2]];
            xtmp[3] = x[csr.col_ind[idx+3]];

            va = vld1q_f32(&(csr.val[idx]));
            vx = vld1q_f32(xtmp);

            vsum = vmulq_f32(va, vx);
            vst1q_f32(xtmp, vsum);
            sum += xtmp[0] + xtmp[1] + xtmp[2] + xtmp[3];
        }
        for(; idx < csr.row_ptr[i+1]; ++idx)
        {
            sum += csr.val[idx] * x[csr.col_ind[idx]];
        }
#else
        for(idx = csr.row_ptr[i]; idx <= csr.row_ptr[i+1] - 2; idx += 2)
        {
            float64x2_t vsum, va, vx;
            FLOAT xtmp[2];

            xtmp[0] = x[csr.col_ind[idx]];
            xtmp[1] = x[csr.col_ind[idx+1]];

			// vector<float64_t>::const_iterator ptr = csr.val.begin();

            va = vld1q_f64(&csr.val[0] + idx);
            vx = vld1q_f64(xtmp);

            vsum = vmulq_f64(va, vx);
            vst1q_f64(xtmp, vsum);
            sum += xtmp[0] + xtmp[1];
        }
        for(; idx < csr.row_ptr[i+1]; ++idx)
        {
            sum += csr.val[idx] * x[csr.col_ind[idx]];
        }
#endif
        y[i] += sum;
    }
}

void execute_SpMV(CSR &csr, int position, int mode)
{

	int col = csr.col;
	FLOAT *x;
	// FLOAT y[col] = {0};
	FLOAT y[col];

	if (mode == 0)
		printf("Execution with scalar\n");
	else if (mode == 1)
		printf("Execution with neon\n");

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
			SpMV_neon(csr, x, y);
	}
	#ifndef NATIVE
	m5_dump_stats(0, 0);
	#endif
	printf("SpMV execution finish.\n\n");

	// free(x);
	// free(y);
}

int main(int argc, char *argv[])
{

	int position; // 0: SPM     1: Memory
	int mode;     // 0: scalar  1: neon

	if (argc != 6)
	{
		printf("Missing parameters.\n");
		exit(1);
	}

	position = atoi(argv[1]);
	mode = atoi(argv[2]);
	CSR asym = assemble_csr_matrix(argv[3]);
	WARMUP_NUM = atoi(argv[4]);
	EXECUTE_NUM = atoi(argv[5]);

	printf("matrix: %s\n", argv[3]);
	printf("rows: %d\tcols: %d\tnnz: %d\n", asym.row, asym.col, asym.nnz);
	printf("WARMUP_NUM: %d\tEXECUTE_NUM: %d\n", WARMUP_NUM, EXECUTE_NUM);

	switch (position)
	{
	case 0:
		execute_SpMV(asym, 0, mode);
		break;
	case 1:
		execute_SpMV(asym, 1, mode);
		break;
	case 2:
		execute_SpMV(asym, 0, mode);
		execute_SpMV(asym, 1, mode);
		break;
	default:
		break;
	}

	return 0;
}
