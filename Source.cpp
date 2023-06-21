#include <iostream>
#include <math.h>
#include <stdlib.h>
#include<string.h>
#include<msclr\marshal_cppstd.h>
#include <ctime>// include this header 
#pragma once
#include <mpi.h>
#using <mscorlib.dll>
#using <System.dll>
#using <System.Drawing.dll>
#using <System.Windows.Forms.dll>

using namespace std;
using namespace msclr::interop;

int* inputImage(int* w, int* h, System::String^ imagePath) //put the size of image in w & h
{
	int* input;


	int OriginalImageWidth, OriginalImageHeight;

	//********************Read Image and save it to local arrayss********	
	//Read Image and save it to local arrayss

	System::Drawing::Bitmap BM(imagePath);

	OriginalImageWidth = BM.Width;
	OriginalImageHeight = BM.Height;
	*w = BM.Width;
	*h = BM.Height;
	int* Red = new int[BM.Height * BM.Width];
	int* Green = new int[BM.Height * BM.Width];
	int* Blue = new int[BM.Height * BM.Width];
	input = new int[BM.Height * BM.Width];
	for (int i = 0; i < BM.Height; i++)
	{
		for (int j = 0; j < BM.Width; j++)
		{
			System::Drawing::Color c = BM.GetPixel(j, i);

			Red[i * BM.Width + j] = c.R;
			Blue[i * BM.Width + j] = c.B;
			Green[i * BM.Width + j] = c.G;

			input[i * BM.Width + j] = ((c.R + c.B + c.G) / 3); //gray scale value equals the average of RGB values

		}

	}
	return input;
}
void createImage(int* image, int width, int height, int index)
{
	System::Drawing::Bitmap MyNewImage(width, height);


	for (int i = 0; i < MyNewImage.Height; i++)
	{
		for (int j = 0; j < MyNewImage.Width; j++)
		{
			//i * OriginalImageWidth + j
			if (image[i * width + j] < 0)
			{
				image[i * width + j] = 0;
			}
			if (image[i * width + j] > 255)
			{
				image[i * width + j] = 255;
			}
			System::Drawing::Color c = System::Drawing::Color::FromArgb(image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j]);
			MyNewImage.SetPixel(j, i, c);
		}
	}
	MyNewImage.Save("..//Data//Output//outputRes" + index + ".PNG");
	std::cout << "result Image Saved " << index << endl;
}

int main()
{
	MPI_Init(NULL, NULL);
	int ImageWidth = 4, ImageHeight = 4;
	int start_s, stop_s, TotalTime = 0;
	int* imageData = nullptr;
	int index;
	int rank, size;
	int* Localarr;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	int localh;
	int lclimage;

	if (rank == 0)
	{
		start_s = clock();

		index = 1200;
		System::String^ imagePath;
		std::string img;

		img = "..//Data//Input//N.png";
		imagePath = marshal_as<System::String^>(img);
		imageData = inputImage(&ImageWidth, &ImageHeight, imagePath);
		start_s = clock();
		ImageWidth = ImageWidth;
		ImageHeight = ImageHeight;
	}

	MPI_Bcast(&ImageWidth, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&ImageHeight, 1, MPI_INT, 0, MPI_COMM_WORLD);

	int* newimg = new int[ImageWidth * ImageHeight];
	localh = ImageHeight / size;      //#of rows each processor will have
	lclimage = localh * ImageWidth;   //#of elements each processor will have

	Localarr = new int[lclimage];
	MPI_Scatter(imageData, lclimage, MPI_INT, Localarr, lclimage, MPI_INT, 0, MPI_COMM_WORLD);

	int kernel[9] = { 0,-1,0,-1,4,-1,0,-1,0 };
	int kernelSize = 3;

	int* localarrayy = new int[localh * ImageWidth];
	for (int r = 0; r < ImageWidth; r++) //each row
	{
		for (int c = 0; c < localh; c++) //each col in row
		{
			int sum, column, row;
			column = ImageWidth * c; //start index of the current column
			row = column + r; //start index of current row in the col 
			sum = 0;

			for (int i = column; i < (column + kernelSize); i++) //loop over every col of kernel
			{
				for (int j = row; j < (row + kernelSize); j++) // loop over all rows of the kernel
				{
					//multiplication of each element in the localarr with its corresponding element in the kernel 
					sum += Localarr[i + (j % ImageWidth)] * kernel[(i - column) * kernelSize + (j - row)];
				}
			}

			localarrayy[row] = sum;
		}
	}
	MPI_Gather(localarrayy, lclimage, MPI_INT, newimg, lclimage, MPI_INT, 0, MPI_COMM_WORLD);
	if (rank == 0)
	{
		createImage(newimg, ImageWidth, ImageHeight, index);
		stop_s = clock();
		TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
		cout << "time: " << TotalTime << endl;
	}
	MPI_Finalize();
	return 0;
}
