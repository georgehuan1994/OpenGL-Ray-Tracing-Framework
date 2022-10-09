//
// Created by George Huan on 2022/10/9.
//

#ifndef APPGUI_H
#define APPGUI_H

// Demonstrate creating a "main" fullscreen menu bar and populating it.
// Note the difference between BeginMainMenuBar() and BeginMenuBar():
// - BeginMenuBar() = menu-bar inside current window (which needs the ImGuiWindowFlags_MenuBar flag!)
// - BeginMainMenuBar() = helper to create menu-bar-sized window at the top of the main viewport + call BeginMenuBar() into it.
static void ShowAppMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            // ShowExampleMenuFile();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X")) {}
            if (ImGui::MenuItem("Copy", "CTRL+C")) {}
            if (ImGui::MenuItem("Paste", "CTRL+V")) {}
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

// Our state
bool show_demo_window = false;
bool show_another_window = false;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

static void ShowDemo(){
    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

//    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
//    {
//        static float f = 0.0f;
//        static int counter = 0;
//
//        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
//
//        ImGui::Text("pointer = %p", 1);
////        ImGui::Text("size = %d x %d", width / 4, height / 4);
//
//        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
//        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
//        ImGui::Checkbox("Another Window", &show_another_window);
//
//        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
//        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
//
//        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
//            counter++;
//        ImGui::SameLine();
//        ImGui::Text("counter = %d", counter);
//
//        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
//        ImGui::End();
//    }

    // 3. Show another simple window.
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }
}

#endif //APPGUI_H
