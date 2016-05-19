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
using namespace Windows::UI::Xaml::Navigation;

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
/// 在应用程序由最终用户正常启动时进行调用。
/// 将在启动应用程序以打开特定文件等情况下使用。
/// </summary>
/// <param name="e">有关启动请求和过程的详细信息。</param>
void App::OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e)
{
#if _DEBUG
    // 调试时显示图形分析信息。
    if (IsDebuggerPresent())
    {
        // 显示当前帧速率计数器
         DebugSettings->EnableFrameRateCounter = true;
    }
#endif
    auto rootFrame = dynamic_cast<Frame^>(Window::Current->Content);

    // 不要在窗口已包含内容时重复应用程序初始化，
    // 只需确保窗口处于活动状态
    if (rootFrame == nullptr)
    {
        // 创建一个 Frame 以用作导航上下文并将其与
        // SuspensionManager 键关联
        rootFrame = ref new Frame();

        rootFrame->NavigationFailed += ref new Windows::UI::Xaml::Navigation::NavigationFailedEventHandler(this, &App::OnNavigationFailed);

        if (e->PreviousExecutionState == ApplicationExecutionState::Terminated)
        {
            // TODO: 仅当适用时还原保存的会话状态，并安排
            // 还原完成后的最终启动步骤

        }

        if (e->PrelaunchActivated == false)
        {
            if (rootFrame->Content == nullptr)
            {
                // 当导航堆栈尚未还原时，导航到第一页，
                // 并通过将所需信息作为导航参数传入来配置
                // 参数
                rootFrame->Navigate(TypeName(MainPage::typeid), e->Arguments);
            }
            // 将框架放在当前窗口中
            Window::Current->Content = rootFrame;
            // 确保当前窗口处于活动状态
            Window::Current->Activate();
        }
    }
    else
    {
        if (e->PrelaunchActivated == false)
        {
            if (rootFrame->Content == nullptr)
            {
                // 当导航堆栈尚未还原时，导航到第一页，
                // 并通过将所需信息作为导航参数传入来配置
                // 参数
                rootFrame->Navigate(TypeName(MainPage::typeid), e->Arguments);
            }
            // 确保当前窗口处于活动状态
            Window::Current->Activate();
        }
    }
}

/// <summary>
/// 在将要挂起应用程序执行时调用。  在不知道应用程序
/// 无需知道应用程序会被终止还是会恢复，
/// 并让内存内容保持不变。
/// </summary>
/// <param name="sender">挂起的请求的源。</param>
/// <param name="e">有关挂起请求的详细信息。</param>
void App::OnSuspending(Object^ sender, SuspendingEventArgs^ e)
{
    (void) sender;  // 未使用的参数
    (void) e;   // 未使用的参数

    //TODO: 保存应用程序状态并停止任何后台活动
}

/// <summary>
/// 导航到特定页失败时调用
/// </summary>
///<param name="sender">导航失败的框架</param>
///<param name="e">有关导航失败的详细信息</param>
void App::OnNavigationFailed(Platform::Object ^sender, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs ^e)
{
    throw ref new FailureException("Failed to load Page " + e->SourcePageType.Name);
}