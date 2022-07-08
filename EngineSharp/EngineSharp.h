#pragma once

#include <msclr\marshal_cppstd.h>

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Drawing;
using namespace System::Drawing::Imaging;
using namespace System::Runtime::InteropServices;
using namespace System::Threading;

using namespace std;
using namespace msclr::interop;

typedef void(*get_current_frame)(std::string result);

void __declspec(dllexport) Init();
void __declspec(dllexport) Free();
int __declspec(dllexport) Run(std::string filename);
void __declspec(dllexport) StopEngine();
void __declspec(dllexport) GetCurrentFrame(unsigned char *buffer);
void __declspec(dllexport) ReceivedHandle(get_current_frame handle);

int __declspec(dllexport) GetFrameWidth();
int __declspec(dllexport) GetFrameHeight();
int __declspec(dllexport) GetFrameChannels();

namespace EngineSharp 
{
	public ref class VehicleCounter
	{
	public:
		delegate void delegate_ReceivedCameraFrame(System::Object ^sender, String^ result);
		event delegate_ReceivedCameraFrame ^ReceivedCameraFrame;

		VehicleCounter()
		{
			Init();

			managed_camera_data = gcnew delegate_frame_data(this, &VehicleCounter::OnFrame);
			GCHandle::Alloc(managed_camera_data);
			handle_on_camera_data = static_cast<get_current_frame>(Marshal::GetFunctionPointerForDelegate(managed_camera_data).ToPointer());
			GC::KeepAlive(managed_camera_data);
		}

		void Start(String^ source)
		{
			ReceivedHandle(handle_on_camera_data);
			string src = marshal_as<string>(source);
			Run(src);
		}

		void Stop()
		{
			StopEngine();
		}

		System::Drawing::Bitmap^ GetFrame()
		{
			try
			{
				int width = GetFrameWidth();
				int height = GetFrameHeight();
				int channels = GetFrameChannels();

				System::Drawing::Rectangle rectangle = System::Drawing::Rectangle(0, 0, width, height);
				Bitmap^ bitmap = gcnew Bitmap(width, height, PixelFormat::Format24bppRgb);
				BitmapData^ bmData = bitmap->LockBits(rectangle, ImageLockMode::WriteOnly, PixelFormat::Format24bppRgb);
				unsigned char *buffer = (unsigned char *)((void*)(bmData->Scan0));

				int length = width * height * 3;
				GetCurrentFrame(buffer);

				bitmap->UnlockBits(bmData);
				return bitmap;
			}
			catch (...)
			{
				return nullptr;
			}
		}

	private:
		delegate void delegate_frame_data(std::string results);
		get_current_frame handle_on_camera_data;
		delegate_frame_data ^managed_camera_data;

		void OnFrame(std::string results)
		{
			System::String^ str = gcnew String(results.c_str());
			ReceivedCameraFrame(this, str);
		}
	};
}
