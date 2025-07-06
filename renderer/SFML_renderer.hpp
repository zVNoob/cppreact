#ifndef _CPPREACT_SFML_RENDERER_HPP
#define _CPPREACT_SFML_RENDERER_HPP

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <iostream>
#include <memory>
#include <optional>

#include "renderer.hpp"

namespace cppreact {
  class SFML_renderer : public renderer {
    SFML_renderer(const SFML_renderer&) = delete;
    SFML_renderer& operator=(const SFML_renderer&) = delete;
    sf::RenderWindow window;
  public:
    SFML_renderer(int width,int height,std::string title) {
      window.create(sf::VideoMode(width,height),title);
      set_size(width,height);
    }
  protected:
    void on_loop(component* root) override {
      window.clear();
      sf::Event evt;
      while (window.pollEvent(evt)) {
        if (evt.type == sf::Event::Closed) {
          exit();
        }
        if (evt.type == sf::Event::Resized) {
          //std::cout << "Resized to " << evt.size.width << "x" << evt.size.height << std::endl;
          set_size(evt.size.width,evt.size.height);
          sf::FloatRect visibleArea(0, 0, evt.size.width, evt.size.height);
          window.setView(sf::View(visibleArea));
          //window.setSize({evt.size.width,evt.size.height});
        }
      }
      render(root);
      window.display();
    }
    void on_rect(bounding_box box,color c,_BlendMode blend) override {
      sf::RectangleShape rect;
      rect.setPosition(sf::Vector2f(box.x,box.y));
      rect.setSize(sf::Vector2f(box.width,box.height));
      sf::Color col = {c.r,c.g,c.b,c.a};
      rect.setFillColor(col);
      window.draw(rect);
    }
    std::any on_new_image(texture* data) override {
      std::shared_ptr<sf::Texture> tex = std::make_shared<sf::Texture>();
      tex->create(data->size().first,data->size().second);
      uint8_t* pixels = new uint8_t[data->size().first * data->size().second * 4];
      for (int i = 0; i < data->size().first * data->size().second * 4; i+=4) {
        pixels[i] = (*data)[i/4/data->size().first][(i/4)%data->size().first].r;
        pixels[i+1] = (*data)[i/4/data->size().first][(i/4)%data->size().first].g;
        pixels[i+2] = (*data)[i/4/data->size().first][(i/4)%data->size().first].b;
        pixels[i+3] = (*data)[i/4/data->size().first][(i/4)%data->size().first].a;
      }
      tex->update(pixels);
      delete[] pixels;
      return std::move(tex);
    }
    void on_image(bounding_box box,std::any& data) override {
      sf::Sprite sprite;
      sf::Texture* tex = std::any_cast<std::shared_ptr<sf::Texture>>(data).get();
      sprite.setTexture(*tex);
      auto [width,height] = tex->getSize();
      sprite.setPosition(sf::Vector2f(box.x,box.y));
      sprite.setScale(sf::Vector2f((float)box.width / width,(float)box.height / height));
      window.draw(sprite);
    }
  };
}

#endif
