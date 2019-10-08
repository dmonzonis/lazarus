#include <lazarus/Graphics/Window.h>

using namespace lz;

Window::Window(int width, int height, Color bg_color/*=Color::Black*/)
    : width(width)
    , height(height)
    , bg_color(bg_color)
{
}

int Window::get_width() const
{
    return width;
}

int Window::get_height() const
{
    return height;
}

void Window::load_tileset(const std::string &path)
{
    tileset.load(path);
    window.create(sf::VideoMode(tileset.get_tile_width() * width,
                                tileset.get_tile_height() * height),
                  "Lazarus");
}

void Window::set_tile(const Position2D &pos, int tile_id, Color color)
{
    if (tile_id < 0 || tile_id >= tileset.get_num_tiles())
    {
        // Tile ID not valid
        return;
    }
    sf::Sprite &sprite = tileset.get_tile(tile_id);
    sprite.setPosition(pos.x * tileset.get_tile_width(), pos.y * tileset.get_tile_height());
    sprite.setColor(color);
    window.draw(sprite);
    return;
}

void Window::render()
{
    // Display contents and clear for future draws
    window.display();
    window.clear(bg_color);
}

bool Window::is_open() const
{
    return window.isOpen();
}

void Window::close()
{
    window.close();
}

bool Window::poll_event(Event &event)
{
    return window.pollEvent(event);
}
