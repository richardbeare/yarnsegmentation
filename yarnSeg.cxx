#include "ioutils.h"
#include "itkConnectedComponentImageFilter.h"
#include "itkChangeLabelImageFilter.h"
#include "itkMorphologicalWatershedFromMarkersImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkMaximumImageFilter.h"
#include "itkAddConstantToImageFilter.h"
#include "itkGradientMagnitudeRecursiveGaussianImageFilter.h"
#include "itkLabelOverlayImageFilter.h"

#include <itkSmartPointer.h>
namespace itk
{
    template <typename T>
    class Instance : public T::Pointer {
    public:
        Instance() : SmartPointer<T>( T::New() ) {}
    };
}

int main(int argc, char*argv[])
{
  const unsigned dim = 2;
  typedef itk::Image<unsigned char, dim> RawImType;
  typedef itk::Image<unsigned short, dim> LabImType;

  RawImType::Pointer raw = readIm<RawImType>(argv[1]);
  RawImType::Pointer darkmarkers = readIm<RawImType>(argv[2]);
  
  itk::Instance < itk::ConnectedComponentImageFilter<RawImType, LabImType> > Labeller;

  Labeller->SetInput(darkmarkers);
  Labeller->SetFullyConnected(true);
  Labeller->Update();
  unsigned bgmarkercount = Labeller->GetObjectCount();

  //writeIm<LabImType>(Labeller->GetOutput(), "labelled.tif");
  
  // stage 1 watershed on the raw image to develop markers for bright regions
  // the markers will be the watershed lines

  itk::Instance< itk::MorphologicalWatershedFromMarkersImageFilter<RawImType, LabImType> > WshedStage1;

  WshedStage1->SetInput(raw);
  WshedStage1->SetMarkerImage(Labeller->GetOutput());
  WshedStage1->SetMarkWatershedLine(true);
  // writeIm<LabImType>(WshedStage1->GetOutput(), "wshed.tif");

  itk::Instance< itk::BinaryThresholdImageFilter<LabImType, RawImType> > SelectLines;
  SelectLines->SetInput(WshedStage1->GetOutput());
  SelectLines->SetLowerThreshold(0);
  SelectLines->SetUpperThreshold(0);
  SelectLines->SetInsideValue(1);
  SelectLines->SetOutsideValue(0);
  writeIm<RawImType>(SelectLines->GetOutput(), "lines.png");

  itk::Instance < itk::ConnectedComponentImageFilter<RawImType, LabImType> > Labeller2;
  Labeller2->SetInput(SelectLines->GetOutput());
  Labeller2->SetFullyConnected(true);

  itk::Instance<itk::AddConstantToImageFilter <LabImType, LabImType::PixelType, LabImType> > AddOffset;
  AddOffset->SetInput(Labeller2->GetOutput());
  AddOffset->SetConstant(bgmarkercount + 1);

  itk::Instance< itk::MaskImageFilter< LabImType, RawImType> > Masker;
  Masker->SetInput(AddOffset->GetOutput());
  Masker->SetInput2(SelectLines->GetOutput());

  itk::Instance< itk::MaximumImageFilter< LabImType, LabImType> > Combiner;
  Combiner->SetInput(Masker->GetOutput());
  Combiner->SetInput2(Labeller->GetOutput());

  itk::Instance< itk::GradientMagnitudeRecursiveGaussianImageFilter <RawImType, RawImType> > Grad;

  // this could be a vertical gradient only
  Grad->SetInput(raw);
  Grad->SetSigma(0.15);

  itk::Instance< itk::MorphologicalWatershedFromMarkersImageFilter<RawImType, LabImType> > WshedStage2;

  WshedStage2->SetInput(Grad->GetOutput());
  WshedStage2->SetMarkerImage(Combiner->GetOutput());
  // don't want the lines this time
  WshedStage2->SetMarkWatershedLine(false);

  writeIm<LabImType>(WshedStage2->GetOutput(), "seg.tif");
  writeIm<RawImType>(Grad->GetOutput(), "grad.tif");

  // save some overlay images for debgugging
  typedef itk::RGBPixel<unsigned char>   RGBPixelType;
  typedef itk::Image<RGBPixelType, dim>    RGBImageType;

  itk::Instance< itk::LabelOverlayImageFilter<RawImType, LabImType, RGBImageType> > LabOverlay;
  LabOverlay->SetInput(raw);
  LabOverlay->SetLabelImage(WshedStage1->GetOutput());
  writeIm<RGBImageType>(LabOverlay->GetOutput(), "stage1_wslines.png");

  LabOverlay->SetLabelImage(WshedStage2->GetOutput());
  writeIm<RGBImageType>(LabOverlay->GetOutput(), "stage2_ws.png");

  std::cout << "bg objects: " << bgmarkercount << std::endl;

}

