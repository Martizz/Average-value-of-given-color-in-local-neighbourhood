#include <ogx/Plugins/EasyPlugin.h>
#include <ogx/Data/Clouds/CloudHelpers.h>
#include <ogx/Data/Clouds/SphericalSearchKernel.h>

using namespace ogx;
using namespace ogx::Data;
using namespace std;

struct Example : public ogx::Plugin::EasyMethod
{
	// parameters
	Data::ResourceID m_node_id;
	ogx::String channel; //R, G or B
	float radius; //radius of local neighbourhood


	// constructor
	Example() : EasyMethod(L"Marta Zawadzka", L"Algorithm calculates average value of given color (R, G or B) in local neighbourhood and saves information in new layer.")
	{
	}

	// add input/output parameters
	virtual void DefineParameters(ParameterBank& bank)
	{
		bank.Add(L"node_id", m_node_id).AsNode();
		bank.Add(L"Channel", channel = L"R", L"Pick a channel").Options({ L"R", L"G", L"B" }).ReadOnly();
		bank.Add(L"Neighborhood radius", radius);
	}

	virtual void Run(Context& context)
	{
		OGX_LINE.Msg(ogx::Level::Info, L"Starting plugin...");

		auto node = context.m_project->TransTreeFindNode(m_node_id);
		if (!node)
			ReportError(L"NodeID not found!");
		
		auto element = node->GetElement();
		if (!element)
			ReportError(L"Element not found");
		

		auto cloud = element->GetData<ogx::Data::Clouds::ICloud>();
		if (!cloud)
			ReportError(L"Cloud not found");
		
		ogx::Data::Clouds::PointsRange pointsRange;
		cloud->GetAccess().GetAllPoints(pointsRange);

		ogx::Data::Clouds::PointsRange sphericalRange; //container for neighbourhood
		std::vector<ogx::Data::Clouds::Color> neighborsColors;

		std::vector<StoredReal> colorValues;
		colorValues.reserve(pointsRange.size());

		int index;
		char chan = channel[0];
		if (chan == 'R') 
			index = 0;

		else if (chan == 'G')
			index = 1;

		
		else if (chan == 'B')
			index = 2;
		
		int progress = 0;

		OGX_LINE.Msg(ogx::Level::Info, L"Calculating neighbors...");
		for (const auto& xyz : ogx::Data::Clouds::RangeLocalXYZConst(pointsRange)) {

			auto searchKernel = ogx::Data::Clouds::SphericalSearchKernel(ogx::Math::Sphere3D(radius, xyz.cast<double>()));
			sphericalRange.clear(); 
			cloud->GetAccess().FindPoints(searchKernel, sphericalRange); //calculating neighbourhood

			neighborsColors.clear();
			sphericalRange.GetColors(neighborsColors);

			float averageColor = 0;
			for (const auto& color : neighborsColors)
				averageColor += (float)color(index, 0);

			float average = averageColor / (float)neighborsColors.size();
			colorValues.emplace_back(average);

			progress++;

			if (!context.Feedback().Update((float)progress / (float)pointsRange.size()))
				ReportError(L"Could not update progress");
			
		}

		//saving calculated information to layer
		auto const layers = cloud->FindLayers(channel);
		auto colorLayer = layers.empty() ? cloud->CreateLayer(channel, 0.0) : layers[0];
		pointsRange.SetLayerVals(colorValues, *colorLayer);

		OGX_LINE.Msg(ogx::Level::Info, L"Average color calculated successfully");
	}
};

OGX_EXPORT_METHOD(Example)
