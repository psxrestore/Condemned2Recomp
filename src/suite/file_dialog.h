// condemned2recomp - ReXGlue Recompiled Project
//
// Cross platform file dialogs for launcher

#pragma once

#if REX_PLATFORM_WIN32
    #include <windows.h>
    #include <commdlg.h>
    #include <shlobj.h>  
#elif REX_PLATFORM_LINUX
    #include <gtk/gtk.h>
#endif
#include <optional>
#include <string>

namespace RexGlueSuite {
    class FileDialog {
        public:
            #if REX_PLATFORM_WIN32
            static std::wstring toWide(const std::string& s) {
                int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
                std::wstring ws(len, 0);
                MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, ws.data(), len);
                return ws;
            }
            #endif

            static std::optional<std::string> OpenFilePicker(){
                #if REX_PLATFORM_WIN32
                    OPENFILENAME ofn;
                    char szFile[260] = {0};

                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.lpstrFile = szFile;
                    ofn.nMaxFile = sizeof(szFile);
                    ofn.lpstrFilter = "ISO Files\0*.iso\0";
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

                    if (GetOpenFileName(&ofn)) {
                        return std::string(szFile);
                    }
                #elif REX_PLATFORM_LINUX
                    GtkWidget *dialog;
                    GtkFileChooser *chooser;

                    dialog = gtk_file_chooser_dialog_new(
                        "Select ISO File", nullptr,
                        GTK_FILE_CHOOSER_ACTION_OPEN,
                        "_Cancel", GTK_RESPONSE_CANCEL,
                        "_Open", GTK_RESPONSE_ACCEPT,
                        nullptr
                    );

                    chooser = GTK_FILE_CHOOSER(dialog);

                    GtkFileFilter *filter = gtk_file_filter_new();
                    gtk_file_filter_set_name(filter, "ISO Files");
                    gtk_file_filter_add_pattern(filter, "*.iso");
                    gtk_file_chooser_add_filter(chooser, filter);

                    std::optional<std::string> result;

                    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
                        char *filename = gtk_file_chooser_get_filename(chooser);
                        result = std::string(filename);
                        g_free(filename);
                    }

                    gtk_widget_destroy(dialog);
                    while (gtk_events_pending()) gtk_main_iteration(); // flush events

                    return result;
                #endif
                return std::nullopt;
            }

            static std::optional<std::string> OpenDirectoryPicker(const std::string& initialPath){
                #if REX_PLATFORM_WIN32
                    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
                    if (FAILED(hr)){
                        return std::nullopt;
                    }

                    IFileDialog* pfd = nullptr;
                    hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
                    if (FAILED(hr)) {
                        CoUninitialize();
                        return std::nullopt;
                    }

                    DWORD options;
                    pfd->GetOptions(&options);
                    pfd->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);

                    IShellItem* psiFolder = nullptr;
                    std::wstring wInitial = toWide(initialPath);
                    if (SUCCEEDED(SHCreateItemFromParsingName(wInitial.c_str(), nullptr, IID_PPV_ARGS(&psiFolder)))) {
                        pfd->SetFolder(psiFolder);
                        psiFolder->Release();
                    }

                    hr = pfd->Show(nullptr);
                    if (FAILED(hr)) {
                        pfd->Release();
                        CoUninitialize();
                        return std::nullopt;
                    }

                    IShellItem* psiResult = nullptr;
                    hr = pfd->GetResult(&psiResult);
                    if (FAILED(hr)) {
                        pfd->Release();
                        CoUninitialize();
                        return std::nullopt;
                    }

                    PWSTR pszPath = nullptr;
                    std::optional<std::string> result;
                    hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
                    if (SUCCEEDED(hr)) {
                        int len = WideCharToMultiByte(CP_UTF8, 0, pszPath, -1, nullptr, 0, nullptr, nullptr);
                        std::string path(len, 0);
                        WideCharToMultiByte(CP_UTF8, 0, pszPath, -1, path.data(), len, nullptr, nullptr);
                        result = path;
                    }

                    CoTaskMemFree(pszPath);
                    psiResult->Release();
                    pfd->Release();
                    CoUninitialize();

                    return result;
                #elif REX_PLATFORM_LINUX
                    GtkWidget *dialog;
                    GtkFileChooser *chooser;
                    dialog = gtk_file_chooser_dialog_new(
                        "Select Assets Folder", nullptr,
                        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                        "_Cancel", GTK_RESPONSE_CANCEL,
                        "_Select", GTK_RESPONSE_ACCEPT,
                        nullptr
                    );

                    chooser = GTK_FILE_CHOOSER(dialog);
                    std::optional<std::string> result;
                    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
                        char *folder = gtk_file_chooser_get_filename(chooser);
                        result = std::string(folder);
                        g_free(folder);
                    }

                    gtk_widget_destroy(dialog);
                    while (gtk_events_pending()) gtk_main_iteration();
                    return result;
                #endif
                return std::nullopt;
            }
        };
}