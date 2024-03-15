#include <nk/app.h>
#include <nk/canvas.h>
#include <nk/hid.h>

int main() {
    NkApp* app = nk::app::create({960, 640, "Demo"});
    NkCanvas* canvas = nk::app::canvas(app);
    NkHID* hid = nk::app::hid(app);
    float rotation = 0.0f;

    while (!nk::app::shouldQuit(app)) {
        nk::app::update(app);

        if (nk::hid::keyClick(hid, NkKeyCode::ESC)) {
            nk::app::quit(app);
        }

        nk::canvas::beginFrame(canvas);
        nk::canvas::pushMatrix(canvas);
        nk::canvas::translate(canvas, nk::hid::mouseX(hid),
                              nk::hid::mouseY(hid));
        nk::canvas::rotate(canvas, rotation);
        nk::canvas::drawRect(canvas, -50.0f, -50.0f, 100.0f, 100.0f,
                             NK_COLOR_RGB_UINT(0xff, 0xff, 0x00));
        nk::canvas::popMatrix(canvas);
        nk::canvas::endFrame(canvas);
        nk::canvas::present(canvas);

        rotation += 1.0f / 60.0f;
    }

    nk::app::destroy(app);
    return 0;
}
