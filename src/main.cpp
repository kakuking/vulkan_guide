#include "utils.h"

#include "renderer.h"

int main() {
    Renderer app;

    app.texture.filenames.push_back("static\\texture_0.jpg");
    app.texture.filenames.push_back("static\\texture_1.jpg");
    app.texture.filenames.push_back("static\\texture_2.jpg"); 

    try {
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}