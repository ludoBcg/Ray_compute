/******************************************************************
*
* utils.h
*
* Based on smallpt by Kevin Beason, released under the MIT License.
* https://www.kevinbeason.com/smallpt/
*
*******************************************************************/

#include <cmath>   
#include <cstdlib> 
#include <iostream>
#include <fstream>
#include <vector>

#define _USE_MATH_DEFINES
#include <math.h>

namespace pathTracing
{

const double eps = 1e-4;

double normRand()
{
    // returns a random double in the [0; 1] range
    return (double)rand() / (double)RAND_MAX;
}

/*
 * Struct for standard Vector operations in 3D 
 * (used for points, vectors, and colors)
 */
struct Vector 
{        
    double x, y, z;           /* Position XYZ or color RGB */

    Vector(const Vector &b) : x(b.x), y(b.y), z(b.z) {}
    Vector() : x(0.0), y(0.0), z(0.0) {}
    Vector(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
    
    Vector operator+(const Vector &b) const             { return Vector(x + b.x, y + b.y, z + b.z); }
    Vector operator-(const Vector &b) const             { return Vector(x - b.x, y - b.y, z - b.z); }
    Vector operator/(double c) const                    { return Vector(x / c, y / c, z / c); }
    Vector operator*(double c) const                    { return Vector(x * c, y * c, z * c); }
    friend Vector operator*(double c, const Vector &b)  { return b * c; }

    Vector MultComponents(const Vector &b) const    { return Vector(x * b.x, y * b.y, z * b.z); } 
    const double Length() const                     { return sqrt(x*x + y*y + z*z); } 
    const Vector Normalized() const                 { return Vector(x, y, z) / sqrt(x*x + y*y + z*z); }
    const double Dot(const Vector &b) const         { return x * b.x + y * b.y + z * b.z; }
    const double Max()                              { return fmax(z, fmax(x, y)); }
    const Vector Cross(const Vector &b) const {
        return Vector((y * b.z) - (z * b.y), 
                      (z * b.x) - (x * b.z), 
                      (x * b.y) - (y * b.x));
    }
    Vector& clamp() {
        x = x<0 ? 0.0 : x>1.0 ? 1.0 : x;
        y = y<0 ? 0.0 : y>1.0 ? 1.0 : y;
        z = z<0 ? 0.0 : z>1.0 ? 1.0 : z;
        return *this;   
    }
};

typedef Vector Color;
const Color BackgroundColor(0.0, 0.0, 0.0);


/*
 * Structure for rays (e.g. viewing ray, path tracing)
 */
struct Ray 
{
    Vector org, dir;    /* Origin and direction */
    Ray(const Vector org_, const Vector &dir_) : org(org_), dir(dir_) {}
};


/*
 * Struct holds pixels/colors of rendered image
 */
struct Image 
{
    int width, height;
    Color *pixels;

    Image(int _w, int _h) : width(_w), height(_h) 
    {
        pixels = new Color[width * height];        
    }

    Color getColor(int x, int y) 
    {
        int image_index = (height-y-1) * width + x; 
        return pixels[image_index];
    }

    void setColor(int x, int y, const Color &c) 
    {
        int image_index = (height-y-1) * width + x; 
        pixels[image_index] = c;
    }

    void addColor(int x, int y, const Color &c) 
    {
        int image_index = (height-y-1) * width + x; 
        pixels[image_index] = pixels[image_index] + c;
    }

    int toInteger(double x)
    { 
        // Clamp to [0,1] 
        if (x < 0.0)
            x = 0.0;          
        else if (x > 1.0)
            x = 1.0;             

        // Apply gamma correction and convert to integer
        return int(pow(x,1/2.2) * 255 + 0.5); 
    }

    void Save(const std::string &filename) 
    {
        /* Save image in PPM format */
        FILE *f = fopen(filename.c_str(), "wb");
        fprintf(f, "P3\n%d %d\n%d\n", width, height, 255);
        for (int i = 0; i < width * height; i++)
            fprintf(f,"%d %d %d ", toInteger(pixels[i].x), 
                                   toInteger(pixels[i].y), 
                                   toInteger(pixels[i].z));
        fclose(f);
    }
};


/*
 * Scene objects are spheres; material either perfectly diffuse, 
 * specular (mirror reflection) or transparent (refraction/reflection)
 * (DIFFuse, SPECular, REFRactive)
 */
enum Refl_t { DIFF, SPEC, REFR }; 


/*
 * Base class for geometry 
 */
class Primitive
{
public:
    Color emission = Color(0.0, 0.0, 0.0);
    Color color = Color(0.0, 0.0, 0.0);
    Refl_t refl = DIFF;

    Primitive() {}

    Primitive(Vector emission_, Vector color_, Refl_t refl_)
        : emission(emission_), color(color_), refl(refl_) 
    {}
};


/*
 * Sphere geometry 
 */
class Sphere : public Primitive
{
public:
    double radius;       
    Vector center; 
   
    
    Sphere(double radius_, Vector center_, Vector emission_, Vector color_, Refl_t refl_)
        : radius(radius_), center(center_), Primitive(emission_, color_, refl_) 
    {}

    double Intersect(const Ray &ray) const 
    { 
        /*
        * Check for ray-sphere intersection by checking if any point along the ray 
        * has a position which distance to sphere center is smaller than radius, 
        * i.e, solving for t:
        *     (( o + t*d ) - c )^2 - R^2 = 0
        *     (d.d)*t^2 + 2*(o-c)*d*t + ((o-c)*(o-c) - R^2) = 0
        * which is a quadratic eq of the form A*t^2 + B*t + C = 0
        * solutions are therefore:
        *     t = ( -B +/- sqrt(B^2 - 4*A*C) ) / (2*A)
        * becomes:
        *     t = b +/- sqrt(b^2 - ((o-c)*(o-c) + R^2)) 
        * with b = (o-c).d 
        */

        Vector c2o = center - ray.org;  // sphere center to ray origin 
        double b = c2o.Dot(ray.dir);
        double radicant = b*b - c2o.Dot(c2o) + radius*radius;
        if (radicant < 0.0)
            return 0.0;                 // No intersection  
        else   
            radicant = sqrt(radicant);
        
        double t = b - radicant;
        if(t > eps)
            return t;
        
        t = b + radicant;
        if(t > eps)
            return t;
        
        return 0.0;                     // No intersection in ray direction
    }
};


/*
 * Triangle geometry 
 */
class Triangle : public Primitive
{
public:
    Vector p0;
    Vector p1;
    Vector p2;
    Vector edge_a, edge_b;
    Vector normal;

    double a_len, b_len;    // Edge lengths

    Triangle(const Vector p0_, const Vector &a_, const Vector &b_, const Color &emission_, const Color &color_) 
        : p0(p0_), edge_a(a_), edge_b(b_), Primitive(emission_, color_, DIFF) 
    {
        normal = edge_a.Cross(edge_b);
        normal = normal.Normalized();        
        a_len = edge_a.Length();
        b_len = edge_b.Length();
        p1 = p0 + edge_a;
        p2 = p0 + edge_b;
    }


    // Triangle-ray intersection 
    const double Intersect(const Ray &ray) 
    {
        // 1. Check for plane-ray intersection
        const double t = (p0 - ray.org).Dot(normal) / ray.dir.Dot(normal);
        if (t <= eps)
            return 0.0;

        // q = plane-ray intersection
        Vector q = ray.org + ray.dir * t;


        /* 2. Determine if p is within triangle
         * Compute barycentric coords (lambda0, lambda1, lambda2 = 1 - lambda0 - lambda1)
         * and check that they are in the [0,1] range
         * Solving : q = lambda0*p0 + lambda1*p1 + (1-lambda0-lambda1)*p2
         *
         * system:       q_x = lambda0*p0_x + lambda1*p1_x + (1-lambda0-lambda1)*p2_x
         *               q_y = lambda0*p0_y + lambda1*p1_y + (1-lambda0-lambda1)*p2_y
         *               q_z = lambda0*p0_z + lambda1*p1_z + (1-lambda0-lambda1)*p2_z
         *           
         *  ->           q_x = lambda0(p0_x - p2_x) + lambda1(p1_x - p2_x) + p2_x
         *               q_y = lambda0(p0_y - p2_y) + lambda1(p1_y - p2_y) + p2_y
         *
         *  ->           lambda0(p0_x - p2_x) + lambda1(p1_x - p2_x) = q_x - p2_x
         *               lambda0(p0_y - p2_y) + lambda1(p1_y - p2_y) = q_y - p2_y
         *
         *  matrix form: |(p0_x - p2_x)  (p1_x - p2_x)| . |lambda0| = |q_x - p2_x|
         *               |(p0_y - p2_y)  (p1_y - p2_y)|   |lambda1|   |q_y - p2_y|
         *                          = matrix T
         *
         *  ->           |lambda0| = (1/detT)|(p1_y - p2_y)  (p2_x - p1_x)| . |q_x - p2_x|
         *               |lambda1|          |(p2_y - p0_y)  (p0_x - p2_x)|    |q_y - p2_y|
         *
         *  ->           lambda0 = (1/detT) * ((p1_y - p2_y)*(q_x - p2_x) + (p2_x - p1_x)*(q_x - p2_x))
         *               lambda1=  (1/detT) * ((p2_y - p0_y)*(q_y - p2_y) + (p0_x - p2_x)*(q_y - p2_y))
         * with:         detT = (p0_x-p2_x)*(p1_y-p2_y) - (p0_y-p2_y)*(p1_x-p2_x)
         *
         * If some coords are identical, use the third line of the system
         */
        
        double lambda_0 = ( (p1.y-p2.y)*(q.x-p2.x) + (p2.x-p1.x)*(q.y-p2.y) ) / ( (p0.x-p2.x)*(p1.y-p2.y) - (p0.y-p2.y)*(p1.x-p2.x) );
        double lambda_1 = ( (p2.y-p0.y)*(q.x-p2.x) + (p0.x-p2.x)*(q.y-p2.y) ) / ( (p0.x-p2.x)*(p1.y-p2.y) - (p0.y-p2.y)*(p1.x-p2.x) );
        
        if( (p0.z == p2.z) && (p1.z == p2.z) )
        {
            lambda_0 = ( (p1.y-p2.y)*(q.x-p2.x) + (p2.x-p1.x)*(q.y-p2.y) ) / ( (p0.x-p2.x)*(p1.y-p2.y) - (p0.y-p2.y)*(p1.x-p2.x) );
            lambda_1 = ( (p2.y-p0.y)*(q.x-p2.x) + (p0.x-p2.x)*(q.y-p2.y) ) / ( (p0.x-p2.x)*(p1.y-p2.y) - (p0.y-p2.y)*(p1.x-p2.x) );
        }
        else if( (p0.x == p2.x) && (p1.x == p2.x) )
        {
            lambda_0 = ( (p1.z-p2.z)*(q.y-p2.y) + (p2.y-p1.y)*(q.z-p2.z) ) / ( (p0.y-p2.y)*(p1.z-p2.z) - (p0.z-p2.z)*(p1.y-p2.y) );
            lambda_1 = ( (p2.z-p0.z)*(q.y-p2.y) + (p0.y-p2.y)*(q.z-p2.z) ) / ( (p0.y-p2.y)*(p1.z-p2.z) - (p0.z-p2.z)*(p1.y-p2.y) );
        }
        else if( (p0.y == p2.y) && (p1.y == p2.y) )
        {
            lambda_0 = ( (p1.z-p2.z)*(q.x-p2.x) + (p2.x-p1.x)*(q.z-p2.z) ) / ( (p0.x-p2.x)*(p1.z-p2.z) - (p0.z-p2.z)*(p1.x-p2.x) );
            lambda_1 = ( (p2.z-p0.z)*(q.x-p2.x) + (p0.x-p2.x)*(q.z-p2.z) ) / ( (p0.x-p2.x)*(p1.z-p2.z) - (p0.z-p2.z)*(p1.x-p2.x) );
        }

        double lambda_2 = 1.0 - lambda_0 - lambda_1;

        if( (lambda_0 > 1.0) || (lambda_0  < 0.0) || (lambda_1 > 1.0) || (lambda_1  < 0.0) || (lambda_2 > 1.0) || (lambda_2  < 0.0) )
            return 0.0;

        return t;

    }  
};


/*
 * Hard-coded scene definition: the geometry is composed of spheres
 * (i.e. Cornell box walls are part of very large spheres). 
 * These are defined by:
 * - radius, center 
 * - emitted light (light sources), surface reflectivity (~color), material
 */
std::vector<Sphere> spheres = 
{
    Sphere( 1e5, Vector( 1e5  +1,      40.8,      81.6),  Vector(), Vector(.75,.25,.25), DIFF), /* Left wall */
    Sphere( 1e5, Vector(-1e5 +99,      40.8,      81.6),  Vector(), Vector(.25,.25,.75), DIFF), /* Rght wall */
    Sphere( 1e5, Vector(      50,      40.8,       1e5),  Vector(), Vector(.75,.75,.75), DIFF), /* Back wall */
    Sphere( 1e5, Vector(      50,      40.8, -1e5 +170),  Vector(), Vector(),            DIFF), /* Front wall */
    Sphere( 1e5, Vector(      50,       1e5,      81.6),  Vector(), Vector(.75,.75,.75), DIFF), /* Floor */
    Sphere( 1e5, Vector(      50,-1e5 +81.6,      81.6),  Vector(), Vector(.75,.75,.75), DIFF), /* Ceiling */

    Sphere(16.5, Vector(27, 16.5, 47), Vector(), Vector(1,1,1)*.999,  SPEC), /* Mirror sphere */
    Sphere(16.5, Vector(73, 16.5, 78), Vector(), Vector(1,1,1)*.999,  REFR), /* Glas sphere */
};

Sphere LightSource( 1.5, Vector(50, 81.6-16.5, 81.6), Vector(4,4,4)*100, Vector(), DIFF);


/*
 * Hard-coded scene definition, triangle version
 */
std::vector<Triangle> triangles = 
{	
    // Cornell Box walls 
    Triangle(Vector(0.0, 0.0, 0.0), Vector(100.0, 0.0, 0.0), Vector(0.0, 80.0, 0.0),   
              Vector(), Color(0.75, 0.75, 0.75)), /* Back */
    Triangle(Vector(100.0, 80.0, 0.0), Vector(-100.0, 0.0, 0.0), Vector(0.0, -80.0, 0.0),   
              Vector(), Color(0.75, 0.75, 0.75)), /* Back */
              
    Triangle(Vector(0.0, 0.0, 170.0), Vector(100.0, 0.0, 0.0), Vector(0.0, 0.0, -170.0), 
              Vector(), Color(0.75, 0.75, 0.75)), /* Bottom */
    Triangle(Vector(100.0, 0.0, 0.0), Vector(-100.0, 0.0, 0.0), Vector(0.0, 0.0, 170.0), 
              Vector(), Color(0.75, 0.75, 0.75)), /* Bottom */
              
    Triangle(Vector(0.0, 80.0, 0.0), Vector(100.0, 0.0, 0.0), Vector(0.0, 0.0, 170.0),  
              Vector(), Color(0.75, 0.75, 0.75)), /* Top */
    Triangle(Vector(100.0, 80.0, 170.0), Vector(-100.0, 0.0, 0.0), Vector(0.0, 0.0, -170.0),  
              Vector(), Color(0.75, 0.75, 0.75)), /* Top */
              
    Triangle(Vector(0.0, 0.0, 170.0), Vector(0.0, 0.0, -170.0), Vector(0.0, 80.0, 0.0),   
              Vector(), Color(0.75, 0.25, 0.25)), /* Left */
    Triangle(Vector(0.0, 80.0, 0.0), Vector(0.0, 0.0, 170.0), Vector(0.0, -80.0, 0.0),   
              Vector(), Color(0.75, 0.25, 0.25)), /* Left */
              
    Triangle(Vector(100.0, 0.0, 0.0), Vector(0.0, 0.0, 170.0), Vector(0.0, 80.0, 0.0),   
              Vector(), Color(0.25, 0.25, 0.75)), /* Right */
    Triangle(Vector(100.0, 80.0, 170.0), Vector(0.0, 0.0, -170.0), Vector(0.0, -80.0, 0.0),   
              Vector(), Color(0.25, 0.25, 0.75)), /* Right */
              
    Triangle(Vector(100.0, 0.0, 170.0), Vector(-100.0, 0.0, 0.0), Vector(0.0, -80.0, 0.0),  
              Vector(), Color(0,1,0)), /* Front (not visible) */
    Triangle(Vector(0.0, -80.0, 170.0), Vector(100.0, 0.0, 0.0), Vector(0.0, 80.0, 0.0),  
              Vector(), Color(0,1,0)), /* Front (not visible) */
	
    // Area light source on top
//    Triangle(Vector(40.0, 79.99, 65.0), Vector(20.0, 0.0, 0.0), Vector(0.0, 0.0, 20.0), 
//              Vector(12,12,12), Color(0.75, 0.75, 0.75)),
//    Triangle(Vector(60.0, 79.99, 85.0), Vector(-20.0, 0.0, 0.0), Vector(0.0, 0.0, -20.0), 
//              Vector(12,12,12), Color(0.75, 0.75, 0.75)),

    // Cuboid in room
    Triangle(Vector(30.0, 0.0, 100.0), Vector(0.0, 0.0, -20.0), Vector(0.0, 40.0, 0.0),   
              Vector(), Color(0.75, 0.75, 0.75)), /* Right */
    Triangle(Vector(30.0, 40.0, 80.0), Vector(0.0, 0.0, 20.0), Vector(0.0, -40.0, 0.0),   
              Vector(), Color(0.75, 0.75, 0.75)), /* Right */
              
    Triangle(Vector(10.0, 0.0, 80.0), Vector(0.0, 0.0, 20.0), Vector(0.0, 40.0, 0.0),   
              Vector(), Color(0.75, 0.75, 0.75)), /* Left */
    Triangle(Vector(10.0, 40.0, 100.0), Vector(0.0, 0.0, -20.0), Vector(0.0, -40.0, 0.0),   
              Vector(), Color(0.75, 0.75, 0.75)), /* Left */
              
    Triangle(Vector(10.0, 0.0, 100.0), Vector(20.0, 0.0, 0.0), Vector(0.0, 40.0, 0.0),   
              Vector(), Color(0.75, 0.75, 0.75)), /* Front */
    Triangle(Vector(30.0, 40.0, 100.0), Vector(-20.0, 0.0, 0.0), Vector(0.0, -40.0, 0.0),   
              Vector(), Color(0.75, 0.75, 0.75)), /* Front */
              
    Triangle(Vector(30.0, 0.0, 80.0), Vector(-20.0, 0.0, 0.0), Vector(0.0, 40.0, 0.0),  
              Vector(), Color(0.75, 0.75, 0.75)), /* Back */
    Triangle(Vector(10.0, 40.0, 80.0), Vector(20.0, 0.0, 0.0), Vector(0.0, -40.0, 0.0),  
              Vector(), Color(0.75, 0.75, 0.75)), /* Back */
              
    Triangle(Vector(10.0, 40.0, 100.0), Vector(20.0, 0.0, 0.0), Vector(0.0, 0.0, -20.0), 
              Vector(), Color(0.75, 0.75, 0.75)), /* Top */
    Triangle(Vector(30.0, 40.0, 80.0), Vector(-20.0, 0.0, 0.0), Vector(0.0, 0.0, 20.0), 
              Vector(), Color(0.75, 0.75, 0.75)), /* Top */
};



} //namespace pathTracing
