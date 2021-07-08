#include "Smooth.h"
#include "vtkPolyDataReader.h"
#include "vtkXMLPolyDataWriter.h"
#include "observe_error.h""
#include "vtkSTLReader.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkPolyDataReader.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkvmtkCapPolyData.h"
#include "vtkPolyDataNormals.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDelaunay3D.h"
#include "vtkDataArray.h"
#include "vtkvmtkInternalTetrahedraExtractor.h"
#include "vtkvmtkVoronoiDiagram3D.h"
#include "vtkImplicitBoolean.h"
#include "vtkSphere.h"
#include "vtkClipPolyData.h"
#include "vtkCleanPolyData.h"
#include "vtkvmtkPolyBallModeller.h"
#include "vtkMarchingCubes.h"
#include "vtkPointData.h"
#include "vtkDoubleArray.h"
#include "vtkPolyDataWriter.h"
#include "vtkSTLWriter.h"

#include <boost/filesystem.hpp>


Smooth::Smooth()
{

}

Smooth::~Smooth()
{

}

void Smooth::SetInputPath(std::string inputPath)
{
	m_inputPath = inputPath;
}

void Smooth::SetOutputPath(std::string outputPath)
{
	m_outputPath = outputPath;
}

void Smooth::SetCenterlinePath(std::string centerlinePath)
{
	m_centerlinePath = centerlinePath;
}

void Smooth::SetSmoothFactor(float smoothFactor)
{
	m_smoothFactor = smoothFactor;
}

void Smooth::SetPolyBallImageSize(int polyBallImageSize)
{
	m_polyBallImageSize = polyBallImageSize;
}

void Smooth::Run()
{
	vtkSmartPointer<ErrorObserver> errorObserver = vtkSmartPointer<ErrorObserver>::New();

	// read input surface
	std::cout << "Reading input surface..." << std::endl;
	std::cout << "Surface path: " << m_inputPath << std::endl;

	std::string input_extension = boost::filesystem::extension(m_inputPath);
	if (input_extension == ".vtp" || input_extension == ".VTP")
	{
		vtkSmartPointer<vtkXMLPolyDataReader> reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
		reader->SetFileName(m_inputPath.c_str());
		reader->AddObserver(vtkCommand::ErrorEvent, errorObserver);
		reader->Update();
		m_input->DeepCopy(reader->GetOutput());
	}
	else if (input_extension == ".stl" || input_extension == ".STL")
	{
		vtkSmartPointer<vtkSTLReader> reader = vtkSmartPointer<vtkSTLReader>::New();
		reader->SetFileName(m_inputPath.c_str());
		reader->AddObserver(vtkCommand::ErrorEvent, errorObserver);
		reader->Update();
		m_input->DeepCopy(reader->GetOutput());
	}
	else if (input_extension == ".vtk" || input_extension == ".VTK")
	{
		vtkSmartPointer<vtkPolyDataReader> reader = vtkSmartPointer<vtkPolyDataReader>::New();
		reader->SetFileName(m_inputPath.c_str());
		reader->AddObserver(vtkCommand::ErrorEvent, errorObserver);
		m_input->DeepCopy(reader->GetOutput());
	}
	else
	{
		std::cerr << "Invalid input data type, only accept STL, VTP or VTK files" << std::endl;
		return;
	}

	// read centerline
	std::cout << "Reading centerline..." << std::endl;
	std::cout << "Centerline path: " << m_centerlinePath << std::endl;

	std::string centerline_extension = boost::filesystem::extension(m_centerlinePath);
	if (centerline_extension == ".vtp" || centerline_extension == ".VTP")
	{
		vtkSmartPointer<vtkXMLPolyDataReader> reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
		reader->SetFileName(m_centerlinePath.c_str());
		reader->AddObserver(vtkCommand::ErrorEvent, errorObserver);
		reader->Update();
		m_centerline->DeepCopy(reader->GetOutput());
	}
	else if (centerline_extension == ".vtk" || centerline_extension == ".VTK")
	{
		vtkSmartPointer<vtkPolyDataReader> reader = vtkSmartPointer<vtkPolyDataReader>::New();
		reader->SetFileName(m_centerlinePath.c_str());
		reader->AddObserver(vtkCommand::ErrorEvent, errorObserver);
		m_centerline->DeepCopy(reader->GetOutput());
	}
	else
	{
		std::cerr << "Invalid centerline data type, only accept VTP or VTK files" << std::endl;
		return;
	}

	// compute smoothed voronoi diagram 
	this->ComputeVoronoiDiagram();
	this->SmoothVoronoiDiagram();

	// write output file
	// check parent path exists
	boost::filesystem::path outputParentDir = boost::filesystem::path(m_outputPath).parent_path();
	
	if (!boost::filesystem::is_directory(outputParentDir))
	{
		boost::filesystem::create_directory(outputParentDir);
	}

	std::string output_extension = boost::filesystem::extension(m_outputPath);
	if (output_extension == ".vtk" || output_extension == ".VTK")
	{
		vtkSmartPointer<vtkPolyDataWriter> writer = vtkSmartPointer<vtkPolyDataWriter>::New();
		writer->SetFileName(m_outputPath.c_str());
		writer->AddObserver(vtkCommand::ErrorEvent, errorObserver);
		writer->SetInputData(m_output);
		writer->Write();
	}
	else if (output_extension == ".stl" || output_extension == ".stl")
	{
		vtkSmartPointer<vtkSTLWriter> writer = vtkSmartPointer<vtkSTLWriter>::New();
		writer->SetFileName(m_outputPath.c_str());
		writer->AddObserver(vtkCommand::ErrorEvent, errorObserver);
		writer->SetInputData(m_output);
		writer->Write();
	}
	else
	{
		vtkSmartPointer<vtkXMLPolyDataWriter> writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
		writer->SetFileName(m_outputPath.c_str());
		writer->AddObserver(vtkCommand::ErrorEvent, errorObserver);
		writer->SetInputData(m_output);
		writer->Write();
	}

	std::cout << "Save smoothed vessel success" << std::endl;
	std::cout << "Output path: " << m_outputPath << std::endl;
}

void Smooth::ComputeVoronoiDiagram()
{
	std::cout << "Computing Voronoi diagram..." << std::endl;

	vtkSmartPointer<vtkvmtkCapPolyData> capper = vtkSmartPointer<vtkvmtkCapPolyData>::New();
	capper->SetInputData(m_input);
	capper->Update();

	vtkSmartPointer<vtkPolyDataNormals> surfaceNormals = vtkSmartPointer<vtkPolyDataNormals>::New();
	surfaceNormals->SetInputData(capper->GetOutput());
	surfaceNormals->SplittingOff();
	surfaceNormals->AutoOrientNormalsOn();
	surfaceNormals->SetFlipNormals(false);
	surfaceNormals->ComputePointNormalsOn();
	surfaceNormals->ConsistencyOn();
	surfaceNormals->Update();

	std::cout << "Performing delaunay tesselation..." << std::endl;

	vtkSmartPointer<vtkUnstructuredGrid> delaunayTessellation = vtkSmartPointer<vtkUnstructuredGrid>::New();

	vtkSmartPointer<vtkDelaunay3D> delaunayTessellator = vtkSmartPointer<vtkDelaunay3D>::New();
	delaunayTessellator->CreateDefaultLocator();
	delaunayTessellator->SetInputConnection(surfaceNormals->GetOutputPort());
	delaunayTessellator->SetTolerance(0.001);
	delaunayTessellator->Update();
	delaunayTessellation->DeepCopy(delaunayTessellator->GetOutput());

	vtkDataArray* normalsArray = surfaceNormals->GetOutput()->GetPointData()->GetNormals();
	delaunayTessellation->GetPointData()->AddArray(normalsArray);

	std::cout << "Extracting internal tetrahedra..." << std::endl;

	vtkSmartPointer<vtkvmtkInternalTetrahedraExtractor> internalTetrahedraExtractor = vtkSmartPointer<vtkvmtkInternalTetrahedraExtractor>::New();
	internalTetrahedraExtractor->SetInputData(delaunayTessellation);
	internalTetrahedraExtractor->SetOutwardNormalsArrayName(normalsArray->GetName());
	internalTetrahedraExtractor->RemoveSubresolutionTetrahedraOn();
	internalTetrahedraExtractor->SetSubresolutionFactor(1e-2); //1.0
	internalTetrahedraExtractor->SetSurface(surfaceNormals->GetOutput());

	if (capper->GetCapCenterIds()->GetNumberOfIds() > 0)
	{
		internalTetrahedraExtractor->UseCapsOn();
		internalTetrahedraExtractor->SetCapCenterIds(capper->GetCapCenterIds());
		internalTetrahedraExtractor->Update();
	}

	delaunayTessellation->DeepCopy(internalTetrahedraExtractor->GetOutput());

	std::cout << "Computing Voronoi diagram..." << std::endl;

	vtkSmartPointer<vtkvmtkVoronoiDiagram3D> voronoiDiagramFilter = vtkSmartPointer<vtkvmtkVoronoiDiagram3D>::New();
	voronoiDiagramFilter->SetInputData(delaunayTessellation);
	voronoiDiagramFilter->SetRadiusArrayName("Radius");
	voronoiDiagramFilter->Update();

	m_voronoi->DeepCopy(voronoiDiagramFilter->GetOutput());
}

void Smooth::SmoothVoronoiDiagram()
{
	// create mask array with spheres and centerline tubes
	vtkSmartPointer<vtkDoubleArray> maskArray = vtkSmartPointer<vtkDoubleArray>::New();
	maskArray->SetNumberOfComponents(1);
	maskArray->SetNumberOfTuples(m_voronoi->GetNumberOfPoints());
	maskArray->SetName("Mask");
	maskArray->FillComponent(0, 0);
	m_voronoi->GetPointData()->AddArray(maskArray);
	m_voronoi->GetPointData()->SetActiveScalars("Mask");

	// smooth the clipped voronoi diagram
	vtkNew<vtkImplicitBoolean> smoothFunction;
	smoothFunction->SetOperationTypeToUnion();
	for (int i = 0; i < m_centerline->GetNumberOfPoints(); i++)
	{
		vtkNew<vtkSphere> sphere;
		sphere->SetCenter(m_centerline->GetPoint(i));
		sphere->SetRadius(m_centerline->GetPointData()->GetArray("Radius")->GetTuple1(i)*(1 - m_smoothFactor));
		smoothFunction->AddFunction(sphere);
	}

	std::cout << "Smoothing clipped Voronoi diagram..." << std::endl;
	smoothFunction->EvaluateFunction(m_voronoi->GetPoints()->GetData(), m_voronoi->GetPointData()->GetArray("Mask"));

	vtkSmartPointer<vtkClipPolyData> clipperSmooth = vtkSmartPointer<vtkClipPolyData>::New();
	clipperSmooth->SetValue(0);
	clipperSmooth->SetInsideOut(true);
	clipperSmooth->GenerateClippedOutputOn();
	clipperSmooth->SetInputData(m_voronoi);
	clipperSmooth->Update();

	vtkSmartPointer<vtkCleanPolyData> cleanerSmooth = vtkSmartPointer<vtkCleanPolyData>::New();
	cleanerSmooth->SetInputData(clipperSmooth->GetOutput());
	cleanerSmooth->Update();

	m_voronoi->DeepCopy(cleanerSmooth->GetOutput());

	// Reconstructing Surface from Voronoi Diagram;
	vtkNew<vtkvmtkPolyBallModeller> modeller;
	modeller->SetInputData(m_voronoi);
	modeller->SetRadiusArrayName("Radius");
	modeller->UsePolyBallLineOff();
	int polyBallImageSize[3] = { m_polyBallImageSize,m_polyBallImageSize,m_polyBallImageSize };
	modeller->SetSampleDimensions(polyBallImageSize);
	modeller->Update();

	vtkNew<vtkMarchingCubes> marchingCube;
	marchingCube->SetInputData(modeller->GetOutput());
	marchingCube->SetValue(0, 0.0);
	marchingCube->Update();

	m_output->DeepCopy(marchingCube->GetOutput());
}
