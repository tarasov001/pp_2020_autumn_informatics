#include <iostream>
#include <vector>
#include <ctime>
#include <mpi.h>
#include "change_contrast.h"

#define CLR 256

using namespace std;

vector<int> set_random_img(int _width, int _high)
{
	int _size_img = _width * _high;
	if (_size_img < 1) throw "Error";
	srand(time(NULL));
	std::vector<int> pic(_size_img);
	for (int i = 0; i < _size_img; i++)
	{
		pic[i] = rand() % CLR;
	}
	return pic;
}

void print_img(std::vector<int> pic, int _high, int _width)
{
	for (int i = 0; i < _high; i++)
	{
		for (int j = 0; j < _width; j++)
			cout << pic[i * _high + j] << "	";
		cout << endl;
	}
}

std::vector<int> changeContrast(std::vector<int> &pic, int _width, int _high, int _correction)
{
	int _size_img = _width * _high;
	if (_size_img < 1) throw "Error";
	std::vector<int> color_pallete(CLR);

	if(_correction == 0) return pic; 

	
	unsigned char value = 0;
	int lAB = 0; 

	for (int i = 0; i < _size_img; i++)
		lAB += pic[i];

	lAB = lAB / _size_img;

	double k = 1.0 + _correction / 100.0;

	for (int i = 0; i < CLR; i++)
	{
		int delta = (int)i - lAB;
		int temp = (int)(lAB + k * delta);

		if (temp < 0)
			temp = 0;

		if (temp >= 255)
			temp = 255;
		color_pallete[i] = (unsigned char)temp;
	}


	for (int i = 0; i < _size_img; i++)
	{
		unsigned char value = pic[i];
		pic[i] = (unsigned char)color_pallete[value];
	}

	return pic;
}

std::vector<int> changeContrastParallel(std::vector<int> &pic, int _width, int _high, int _correction)
{
	int _size_img = _width * _high;
	if (_width * _high != pic.size()) throw "Error";
	if (_size_img < 1) throw "Error";
	if(_correction == 0) return pic; 
	int mpisize, mpirank;
	
	MPI_Comm_rank(MPI_COMM_WORLD, &mpirank);
	MPI_Comm_size(MPI_COMM_WORLD, &mpisize);

	int dist = _size_img / mpisize;
	int rem = _size_img % mpisize;
	int mpiroot = 0;

	if (dist < 1)
	{
		if (mpirank == mpiroot) changeContrast(pic, _width, _high , _correction);
		return pic;
	}

	std::vector<int> pic_sbuf = pic;
	std::vector<int> pic_scount(mpisize, 0);
	std::vector<int> pic_displs(mpisize, 0);

	std::vector<int>color_pallete(CLR);
	int lAB = 0;

	if (rem != 0) pic_scount[0] = dist + rem;
	else pic_scount[0] = dist;
	for (int i = 1; i < pic_scount.size(); i++)
		pic_scount[i] = dist;

	for (int i = 1; i < pic_displs.size(); i++)
		pic_displs[i] = pic_displs[i - 1] + pic_scount[i - 1];

	int scount_size;
	MPI_Scatter(&pic_sbuf[0], 1, MPI_INT, &scount_size, 1, MPI_INT, mpiroot, MPI_COMM_WORLD);
	std::vector<int> rec_pic(scount_size, 0);
	MPI_Scatterv(&pic_sbuf[0], &pic_scount[0], &pic_displs[0], MPI_INT, &rec_pic[0], pic_scount[mpirank], MPI_INT, mpiroot, MPI_COMM_WORLD);
	
	int rec_lAB = 0;
	
	for (int i = 0; i < rec_pic.size(); i++)
		rec_lAB += rec_pic[i];

	rec_lAB = rec_lAB / rec_pic.size();
	
	MPI_Reduce(&rec_lAB, &lAB, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	
	if (mpirank == mpiroot)
	{
		double k = 1.0 + _correction / 100.0;

		for (int i = 0; i < CLR; i++)
		{
			int delta = (int)i - lAB;
			int temp = (int)(lAB + k * delta);

			if (temp < 0)
				temp = 0;

			if (temp >= 255)
				temp = 255;
			color_pallete[i] = (unsigned char)temp;
		}

	}
	
	MPI_Bcast(&color_pallete[0], CLR, MPI_INT, 0, MPI_COMM_WORLD);

	for (int i = 0; i < rec_pic.size(); i++)
	{
		unsigned char value = rec_pic[i];
		rec_pic[i] = (unsigned char)color_pallete[value];
	}

	MPI_Gatherv(&rec_pic[0], pic_scount[mpirank], MPI_INT, &pic[0], &pic_scount[0], &pic_displs[0], MPI_INT, mpiroot, MPI_COMM_WORLD);


	return pic;
}