#include "software_renderer.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

#include "triangulation.h"

using namespace std;

namespace CMU462 {


// Implements SoftwareRenderer //

void SoftwareRendererImp::draw_svg( SVG& svg ) {

  // set top level transformation
  transformation = svg_2_screen;

  // draw all elements
  for ( size_t i = 0; i < svg.elements.size(); ++i ) {
    draw_element(svg.elements[i]);
  }

  // draw canvas outline
  Vector2D a = transform(Vector2D(    0    ,     0    )); a.x--; a.y--;
  Vector2D b = transform(Vector2D(svg.width,     0    )); b.x++; b.y--;
  Vector2D c = transform(Vector2D(    0    ,svg.height)); c.x--; c.y++;
  Vector2D d = transform(Vector2D(svg.width,svg.height)); d.x++; d.y++;

  rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
  rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
  rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
  rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

  // resolve and send to render target
  resolve();

}

void SoftwareRendererImp::set_sample_rate( size_t sample_rate ) {
  // called when user press "1""2""3"...
  // Task 4: 
  // You may want to modify this for supersampling support
  this->sample_rate = sample_rate;
  this->w = this->target_w * this->sample_rate;
  this->h = this->target_h * this->sample_rate;
  this->clear_sample_buffer();
}

void SoftwareRendererImp::set_render_target( unsigned char* render_target,
                                             size_t width, size_t height ) {
  // called while resizing
  // Task 4: 
  // You may want to modify this for supersampling support
  this->render_target = render_target;
  this->target_w = width;
  this->target_h = height;

  this->w = width * this->sample_rate;
  this->h = height * this->sample_rate;
  this->clear_sample_buffer();

}

void SoftwareRendererImp::draw_element( SVGElement* element ) {

  // Task 5 (part 1):
  // Modify this to implement the transformation stack

  switch(element->type) {
    case POINT:
      draw_point(static_cast<Point&>(*element));
      break;
    case LINE:
      draw_line(static_cast<Line&>(*element));
      break;
    case POLYLINE:
      draw_polyline(static_cast<Polyline&>(*element));
      break;
    case RECT:
      draw_rect(static_cast<Rect&>(*element));
      break;
    case POLYGON:
      draw_polygon(static_cast<Polygon&>(*element));
      break;
    case ELLIPSE:
      draw_ellipse(static_cast<Ellipse&>(*element));
      break;
    case IMAGE:
      draw_image(static_cast<Image&>(*element));
      break;
    case GROUP:
      draw_group(static_cast<Group&>(*element));
      break;
    default:
      break;
  }

}


// Primitive Drawing //

void SoftwareRendererImp::draw_point( Point& point ) {

  Vector2D p = transform(point.position);
  rasterize_point( p.x, p.y, point.style.fillColor );

}

void SoftwareRendererImp::draw_line( Line& line ) { 

  Vector2D p0 = transform(line.from);
  Vector2D p1 = transform(line.to);
  rasterize_line( p0.x, p0.y, p1.x, p1.y, line.style.strokeColor );

}

void SoftwareRendererImp::draw_polyline( Polyline& polyline ) {

  Color c = polyline.style.strokeColor;

  if( c.a != 0 ) {
    int nPoints = polyline.points.size();
    for( int i = 0; i < nPoints - 1; i++ ) {
      Vector2D p0 = transform(polyline.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polyline.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_rect( Rect& rect ) {

  Color c;
  
  // draw as two triangles
  float x = rect.position.x;
  float y = rect.position.y;
  float w = rect.dimension.x;
  float h = rect.dimension.y;

  Vector2D p0 = transform(Vector2D(   x   ,   y   ));
  Vector2D p1 = transform(Vector2D( x + w ,   y   ));
  Vector2D p2 = transform(Vector2D(   x   , y + h ));
  Vector2D p3 = transform(Vector2D( x + w , y + h ));
  
  // draw fill
  c = rect.style.fillColor;
  if (c.a != 0 ) {
    rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    rasterize_triangle( p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c );
  }

  // draw outline
  c = rect.style.strokeColor;
  if( c.a != 0 ) {
    rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    rasterize_line( p1.x, p1.y, p3.x, p3.y, c );
    rasterize_line( p3.x, p3.y, p2.x, p2.y, c );
    rasterize_line( p2.x, p2.y, p0.x, p0.y, c );
  }

}

void SoftwareRendererImp::draw_polygon( Polygon& polygon ) {

  Color c;

  // draw fill
  c = polygon.style.fillColor;
  if( c.a != 0 ) {

    // triangulate
    vector<Vector2D> triangles;
    triangulate( polygon, triangles );

    // draw as triangles
    for (size_t i = 0; i < triangles.size(); i += 3) {
      Vector2D p0 = transform(triangles[i + 0]);
      Vector2D p1 = transform(triangles[i + 1]);
      Vector2D p2 = transform(triangles[i + 2]);
      rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    }
  }

  // draw outline
  c = polygon.style.strokeColor;
  if( c.a != 0 ) {
    int nPoints = polygon.points.size();
    for( int i = 0; i < nPoints; i++ ) {
Vector2D p0 = transform(polygon.points[(i + 0) % nPoints]);
Vector2D p1 = transform(polygon.points[(i + 1) % nPoints]);
rasterize_line(p0.x, p0.y, p1.x, p1.y, c);
    }
  }
}

void SoftwareRendererImp::draw_ellipse(Ellipse& ellipse) {

    // Extra credit 

}

void SoftwareRendererImp::draw_image(Image& image) {

    Vector2D p0 = transform(image.position);
    Vector2D p1 = transform(image.position + image.dimension);

    rasterize_image(p0.x, p0.y, p1.x, p1.y, image.tex);
}

void SoftwareRendererImp::draw_group(Group& group) {

    for (size_t i = 0; i < group.elements.size(); ++i) {
        draw_element(group.elements[i]);
    }

}

// Rasterization //

// The input arguments in the rasterization functions 
// below are all defined in screen space coordinates

void SoftwareRendererImp::rasterize_point(float x, float y, Color color) {

    // fill in the nearest pixel
    int sx = (int)floor(x);
    int sy = (int)floor(y);

    // check bounds
    if (sx < 0 || sx >= target_w) return;
    if (sy < 0 || sy >= target_h) return;

    //// fill sample - NOT doing alpha blending!
    render_target[4 * (sx + sy * target_w)] = (uint8_t)(color.r * 255);
    render_target[4 * (sx + sy * target_w) + 1] = (uint8_t)(color.g * 255);
    render_target[4 * (sx + sy * target_w) + 2] = (uint8_t)(color.b * 255);
    render_target[4 * (sx + sy * target_w) + 3] = (uint8_t)(color.a * 255);
   
    fill_pixel(sx, sy, color);
}

void SoftwareRendererImp::rasterize_line(float x0, float y0,
    float x1, float y1,
    Color color) {

    // Task 2: 
    // Implement line rasterization
    x0 = x0 * sample_rate;
    y0 = y0 * sample_rate;
    x1 = x1 * sample_rate;
    y1 = y1 * sample_rate;
    
    bool steep = (x0 - x1 == 0 || (y1 - y0) / (x1 - x0) > 1 || (y1 - y0) / (x1 - x0) < -1);
    if (steep) {
        swap(x0, y0);
        swap(x1, y1);
    }
    if (x1 < x0) {
        swap(x0, x1);
        swap(y0, y1);
    }

    std::cout << steep << std::endl;
    float eps_hash = 0;

    float m = (y1 - y0) / (x1 - x0);
    if (m <= 0 && !steep) {
        for (float x = x0, y = y0; x < x1; x++) {
            //rasterize_point(x, y, color);
            fill_sample(x, y, color);
            if (2 * (eps_hash + m) > 1) {
                eps_hash += m;
            }
            else {
                y -= 1;
                eps_hash = eps_hash + m + 1;
            }
        }
    }
    else if (m <= 0 && steep) {
        for (float x = x0, y = y0; x < x1; x++) {
            //rasterize_point(y, x, color);
            fill_sample(y, x, color);
            if (2 * (eps_hash + m) > 1) {
                eps_hash += m;
            }
            else {
                y -= 1;
                eps_hash = eps_hash + m + 1;
            }
        }
    }
    else if (m > 0 && !steep) {
        for (float x = x0, y = y0; x < x1; x++) {
            //rasterize_point(x, y, color);
            fill_sample(x, y, color);
            if (2 * (eps_hash + m) < 1) {
                eps_hash += m;
            }
            else {
                y += 1;
                eps_hash = eps_hash + m - 1;
            }
        }
    }
    else if (m > 0 && steep) {
        for (float x = x0, y = y0; x < x1; x++) {
            //rasterize_point(y, x, color);
            fill_sample(y, x, color);
            if (2 * (eps_hash + m) < 1) {
                eps_hash += m;
            }
            else {
                y += 1;
                eps_hash = eps_hash + m - 1;
            }
        }
    }
}

inline float leftOrRight(float x, float y, float Vx, float Vy, float x_vertice, float y_vertice) {
    return (x - x_vertice) * Vy - (y - y_vertice) * Vx;
}

inline float test_line(float x, float y, float dy, float dx, float yi, float xi)
{
    return (x - xi) * dy - (y - yi) * dx; // N * P < 0
}
void SoftwareRendererImp::rasterize_triangle( float x0, float y0,
                                              float x1, float y1,
                                              float x2, float y2,
                                              Color color ) {
  // Task 3: 
  // Implement triangle rasterization
    x0 = x0 * sample_rate;
    y0 = y0 * sample_rate;
    x1 = x1 * sample_rate;
    y1 = y1 * sample_rate;
    x2 = x2 * sample_rate;
    y2 = y2 * sample_rate;

    float Vy_10 = y1 - y0;
    float Vx_10 = x1 - x0;
    float Vy_21 = y2 - y1;
    float Vx_21 = x2 - x1;
    float Vy_02 = y0 - y2;
    float Vx_02 = x0 - x2;

    float Xmax = max({ x0,x1,x2 });
    float Ymax = max({ y0,y1,y2 });
    float Xmin = floor(min({ x0,x1,x2 })) + 0.5f;
    float Ymin = floor(min({ y0,y1,y2 })) + 0.5f;

    for (float y = Ymin; y <= Ymax; y += 1) {
        bool isTriangle = false;
        for (float x = Xmin; x <= Xmax; x += 1) {
            float t1 = leftOrRight(x, y, Vx_10, Vy_10, x0, y0);
            float t2 = leftOrRight(x, y, Vx_21, Vy_21, x1, y1);
            float t3 = leftOrRight(x, y, Vx_02, Vy_02, x2, y2);

            if ((t1 > 0 && t2 > 0 && t3 > 0) || (t1 < 0 && t2 < 0 && t3 < 0)) {
                //rasterize_point(x, y, color);
                fill_sample(x, y, color);
            }
        }
    }
}

void SoftwareRendererImp::rasterize_image( float x0, float y0,
                                           float x1, float y1,
                                           Texture& tex ) {
  // Task 6: 
  // Implement image rasterization

}

// resolve samples to render target
void SoftwareRendererImp::resolve( void ) {

  // Task 4: 
  // Implement supersampling
  // You may also need to modify other functions marked with "Task 4".
    int x, y, Xshift, Yshift, Xsample, Ysample, Xidx, Yidx;

    float totalR, totalG, totalB, totalA;
    for (x = 0; x < this->w; x += sample_rate) {
        for (y = 0; y < this->h; y += sample_rate) {
            totalR = 0;
            totalG = 0;
            totalB = 0;
            totalA = 0;

            for (Xshift = 0; Xshift < sample_rate; Xshift++) {
                for (Yshift = 0; Yshift < sample_rate; Yshift++) {
                    Xsample = x + Xshift;
                    Ysample = y + Yshift;

                    totalR += sample_buffer[4 * (Xsample + (Ysample * this->w))];
                    totalG += sample_buffer[4 * (Xsample + (Ysample * this->w)) + 1];
                    totalB += sample_buffer[4 * (Xsample + (Ysample * this->w)) + 2];
                    totalA += sample_buffer[4 * (Xsample + (Ysample * this->w)) + 3];

                }
            }
            Xidx = floor(x / sample_rate);
            Yidx = floor(y / sample_rate);

            render_target[4 * (Xidx + Yidx * target_w)] = totalR / pow(sample_rate, 2);
            render_target[4 * (Xidx + Yidx * target_w) + 1] = totalG / pow(sample_rate, 2);
            render_target[4 * (Xidx + Yidx * target_w) + 2] = totalB / pow(sample_rate, 2);
            render_target[4 * (Xidx + Yidx * target_w) + 3] = totalA / pow(sample_rate, 2);
        }
    }

    this->clear_sample_buffer();

  return;
}

void SoftwareRendererImp::fill_sample(int sx, int sy, const Color& c) {

    if (sx < 0 || sx >= w) return;
    if (sy < 0 || sy >= h) return;
    // alpha blending
    Color origin, target;

    origin.r = (float)sample_buffer[4 * (sx + sy * w)] / 255;
    origin.g = (float)sample_buffer[4 * (sx + sy * w) + 1] / 255;
    origin.b = (float)sample_buffer[4 * (sx + sy * w) + 2] / 255;
    origin.a = (float)sample_buffer[4 * (sx + sy * w) + 3] / 255;

    target.r = (1 - c.a) * origin.r + c.a * c.r;
    target.g = (1 - c.a) * origin.g + c.a * c.g;
    target.b = (1 - c.a) * origin.b + c.a * c.b;
    target.a = 1 - (1 - origin.a) * (1 - c.a);

    sample_buffer[4 * (sx + sy * w)] = (uint8_t)(target.r * 255);
    sample_buffer[4 * (sx + sy * w) + 1] = (uint8_t)(target.g * 255);
    sample_buffer[4 * (sx + sy * w) + 2] = (uint8_t)(target.b * 255);
    sample_buffer[4 * (sx + sy * w) + 3] = (uint8_t)(target.a * 255);
}


void SoftwareRendererImp::fill_pixel(int sx, int sy, const Color& c) {

    if (sx < 0 || sx >= target_w) return;
    if (sy < 0 || sy >= target_h) return;
    // alpha blending
    Color origin, target;

    origin.r = (float)render_target[4 * (sx + sy * target_w)] / 255;
    origin.g = (float)render_target[4 * (sx + sy * target_w) + 1] / 255;
    origin.b = (float)render_target[4 * (sx + sy * target_w) + 2] / 255;
    origin.a = (float)render_target[4 * (sx + sy * target_w) + 3] / 255;

    target.r = (1 - c.a) * origin.r + c.a * c.r;
    target.g = (1 - c.a) * origin.g + c.a * c.g;
    target.b = (1 - c.a) * origin.b + c.a * c.b;
    target.a = 1 - (1 - origin.a) * (1 - c.a);

    render_target[4 * (sx + sy * target_w)] = (uint8_t)(target.r * 255);
    render_target[4 * (sx + sy * target_w) + 1] = (uint8_t)(target.g * 255);
    render_target[4 * (sx + sy * target_w) + 2] = (uint8_t)(target.b * 255);
    render_target[4 * (sx + sy * target_w) + 3] = (uint8_t)(target.a * 255);
}

} // namespace CMU462
