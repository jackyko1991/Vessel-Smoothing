#ifndef __SMOOTH_H__
#define __SMOOTH_H__

#include "iostream"
#include "vtkSmartPointer.h"
#include "vtkPolyData.h"

class Smooth
{
public:
	Smooth();
	~Smooth();

	void SetInputPath(std::string);
	void SetOutputPath(std::string);
	void SetCenterlinePath(std::string);
	void SetSmoothFactor(float);
	void SetPolyBallImageSize(int);
	void Run();

private:
	std::string m_inputPath;
	std::string m_centerlinePath;
	std::string m_outputPath;
	float m_smoothFactor = 0.6;
	int m_polyBallImageSize = 90;

	vtkSmartPointer<vtkPolyData> m_input = vtkSmartPointer<vtkPolyData>::New();
	vtkSmartPointer<vtkPolyData> m_centerline = vtkSmartPointer<vtkPolyData>::New();
	vtkSmartPointer<vtkPolyData> m_voronoi = vtkSmartPointer<vtkPolyData>::New();
	vtkSmartPointer<vtkPolyData> m_output = vtkSmartPointer<vtkPolyData>::New();

	void ComputeVoronoiDiagram();
	void SmoothVoronoiDiagram();
};

#endif