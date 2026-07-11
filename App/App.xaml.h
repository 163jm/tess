#pragma once

#include "App.xaml.g.h"

namespace winrt::NativePlayer::implementation {

struct App : AppT<App> {
    App();

    void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

private:
    winrt::Microsoft::UI::Xaml::Window window_{ nullptr };
};

} // namespace winrt::NativePlayer::implementation

namespace winrt::NativePlayer::factory_implementation {
struct App : AppT<App, implementation::App> {};
}
