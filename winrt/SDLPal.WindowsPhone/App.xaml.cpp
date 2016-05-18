//
// App.xaml.cpp
// App 类的实现。
//

#include "pch.h"
#include "MainPage.xaml.h"

using namespace SDLPal;

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Media::Animation;
using namespace Windows::UI::Xaml::Navigation;

// “空白应用程序”模板在 http://go.microsoft.com/fwlink/?LinkID=391641 上有介绍

/// <summary>
/// 初始化单一实例应用程序对象。这是执行的创作代码的第一行，
/// 已执行，逻辑上等同于 main() 或 WinMain()。
/// </summary>
App::App()
{
	InitializeComponent();
	Suspending += ref new SuspendingEventHandler(this, &App::OnSuspending);
}

/// <summary>
/// 在最终用户正常启动应用程序时调用。将在启动应用程序
/// 当启动应用程序以打开特定的文件或显示时使用
/// 搜索结果等
/// </summary>
/// <param name="e">有关启动请求和过程的详细信息。</param>
void App::OnLaunched(LaunchActivatedEventArgs^ e)
{
#if _DEBUG
	if (IsDebuggerPresent())
	{
		DebugSettings->EnableFrameRateCounter = true;
	}
#endif

	auto rootFrame = dynamic_cast<Frame^>(Window::Current->Content);

	// 不要在窗口已包含内容时重复应用程序初始化，
	// 只需确保窗口处于活动状态。
	if (rootFrame == nullptr)
	{
		// 创建一个 Frame 以用作导航上下文并将其与
		// SuspensionManager 键关联
		rootFrame = ref new Frame();

		// TODO: 将此值更改为适合您的应用程序的缓存大小。
		rootFrame->CacheSize = 1;

		//设置默认语言
		rootFrame->Language = Windows::Globalization::ApplicationLanguages::Languages->GetAt(0);

		if (e->PreviousExecutionState == ApplicationExecutionState::Terminated)
		{
			// TODO: 仅当适用时还原保存的会话状态，并安排
			// 还原完成后的最终启动步骤。
		}

		// 将框架放在当前窗口中
		Window::Current->Content = rootFrame;
	}

	if (rootFrame->Content == nullptr)
	{
		// 删除用于启动的旋转门导航。
		if (rootFrame->ContentTransitions != nullptr)
		{
			_transitions = ref new TransitionCollection();
			for (auto transition : rootFrame->ContentTransitions)
			{
				_transitions->Append(transition);
			}
		}

		rootFrame->ContentTransitions = nullptr;
		_firstNavigatedToken = rootFrame->Navigated += ref new NavigatedEventHandler(this, &App::RootFrame_FirstNavigated);

		// 当导航堆栈尚未还原时，导航到第一页，
		// 并通过将所需信息作为导航参数传入来配置
		// 新页面。
		if (!rootFrame->Navigate(MainPage::typeid, e->Arguments))
		{
			throw ref new FailureException("Failed to create initial page");
		}
	}

	// 确保当前窗口处于活动状态
	Window::Current->Activate();
}

/// <summary>
/// 启动应用程序后还原内容转换。
/// </summary>
void App::RootFrame_FirstNavigated(Object^ sender, NavigationEventArgs^ e)
{
	auto rootFrame = safe_cast<Frame^>(sender);

	TransitionCollection^ newTransitions;
	if (_transitions == nullptr)
	{
		newTransitions = ref new TransitionCollection();
		newTransitions->Append(ref new NavigationThemeTransition());
	}
	else
	{
		newTransitions = _transitions;
	}

	rootFrame->ContentTransitions = newTransitions;
	rootFrame->Navigated -= _firstNavigatedToken;
}

void SDLPal::App::OnActivated(Windows::ApplicationModel::Activation::IActivatedEventArgs ^ args)
{
	auto main_page = static_cast<MainPage^>(_main_page);
	switch (args->Kind)
	{
	case ActivationKind::PickFolderContinuation:
	{
		auto folder = safe_cast<IFolderPickerContinuationEventArgs^>(args)->Folder;
		if (folder) main_page->SetPath(folder);
		break;
	}
	case ActivationKind::PickSaveFileContinuation:
	{
		auto save_args = safe_cast<IFileSavePickerContinuationEventArgs^>(args);
		if (save_args->File && save_args->ContinuationData->HasKey("Slot"))
			main_page->Export(save_args->File, safe_cast<Platform::String^>(save_args->ContinuationData->Lookup("Slot")));
		break;
	}
	case ActivationKind::PickFileContinuation:
	{
		auto open_args = safe_cast<IFileOpenPickerContinuationEventArgs^>(args);
		if (open_args->Files->Size > 0 && open_args->ContinuationData->HasKey("Slot"))
			main_page->Import(open_args->Files->First()->Current, safe_cast<Platform::String^>(open_args->ContinuationData->Lookup("Slot")));
		break;
	}
	}
	Application::OnActivated(args);
}

/// <summary>
/// 在将要挂起应用程序执行时调用。将保存应用程序状态
/// 无需知道应用程序会被终止还是会恢复，
/// 并让内存内容保持不变。
/// </summary>
void App::OnSuspending(Object^ sender, SuspendingEventArgs^ e)
{
	(void) sender;	// 未使用的参数
	(void) e;		// 未使用的参数

	// TODO: 保存应用程序状态并停止任何后台活动
}

void App::SetMainPage(Windows::UI::Xaml::Controls::Page^ page)
{
	_main_page = page;
}
