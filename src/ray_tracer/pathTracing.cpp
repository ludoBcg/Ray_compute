/*****************************************************************************************
*
* PathTracing.cpp
*
* This program demonstrates global illumination rendering based on the path tracing method.
* The intergral in the rendering equation is approximated via Monte-Carlo integration.
* Explicit direct lighting is included to improve quality.
* The rendered image is saved in PPM format.
*
* Based on smallpt by Kevin Beason, released under the MIT License.
* https://www.kevinbeason.com/smallpt/
*
*******************************************************************************************/

#include "utils.h"

namespace pathTracing
{
    // Thin lens parameters
    const double aperture = 2.0;       // radius of lens (no depth of field if set to zero) 
    const double focal_depth = 65.0;   // distance between center of lens anf focal plane

    // Scene geometry to render (spheres or triangles)
    const bool useTriangles = true;

    // Path Tracing parameters
    const unsigned int maxDepth = 5;
    const unsigned int nbSamples = 50;


    
    bool IntersectLightSource(const Ray& ray)
    {
        double d = LightSource.Intersect(ray);

        if (d > 0.0)
            return true;

        return false;
    }

    /*
     * Check for closest intersection of a ray with the scene;
     * returns true if intersection is found, as well as ray parameter
     * of intersection and id of intersected object
     */
    bool IntersectSpheres(const Ray& ray, double& t, int& id)
    {
        t = 1e20;

        for (int i = 0; i < spheres.size(); i++)
        {
            double d = spheres.at(i).Intersect(ray);

            if (d > 0.0 && d < t)
            {
                t = d;
                id = i;
            }
        }
        return t < 1e20;
    }

    bool IntersectTriangles(const Ray& ray, double& t, int& id)
    {
        t = 1e20;

        for (int i = 0; i < triangles.size(); i++)
        {
            double d = triangles.at(i).Intersect(ray);
            if (d > 0.0 && d < t)
            {
                t = d;
                id = i;
            }
        }
        return t < 1e20;
    }


    /*
     * Simulates depth-of-field using a thin lens model
     */
    void ThinLens(Ray& _ray)
    {
        //depth of field
        //random polar coords
        double angle = normRand() * 2.0 * M_PI;
        double radius = normRand();
        //build offset vector to generate a new random origin position in the lens
        Vector aperture_offset((cos(angle) * radius) * aperture, (sin(angle) * radius) * aperture, 0.0);
        // new start position
        _ray.org = _ray.org + aperture_offset;
        // new direction
        _ray.dir = _ray.dir * focal_depth - aperture_offset;
        _ray.dir = _ray.dir.Normalized();
        
    }



    /*
    * Recursive path tracing for computing radiance via Monte-Carlo
    * integration; only considers perfectly diffuse, specular or
    * transparent materials;
    * After N bounces Russian Roulette is used to possibly terminate rays;
    * Emitted light from light source only included on first direct hit
    * (possibly via specular reflection, refraction), controlled by
    * parameter E = 0/1;
    * on diffuse surfaces light sources are explicitely sampled;
    * for transparent objects, Schlick´s approximation is employed;
    * for first 3 bounces obtain reflected and refracted component,
    * afterwards one of the two is chosen randomly
    */
    Color Radiance(const Ray& _ray, int _depth, int _E)
    {
        _depth++;

        double t;
        int id = 0;

        // If no intersection with scene, return background color
        if (useTriangles)
            if (!IntersectTriangles(_ray, t, id))   
                return BackgroundColor;

        if (!useTriangles)
            if (!IntersectSpheres(_ray, t, id))
                return BackgroundColor;

        Primitive obj;
        Vector normal(1.0, 0.0, 0.0);

        Vector hitpoint = _ray.org + _ray.dir * t;    // Intersection position
        Color col;

        // Get normal and color at intersection
        if (useTriangles)
        {
            normal = dynamic_cast<Triangle&>(triangles.at(id)).normal.Normalized();
            col = dynamic_cast<Triangle&>(triangles.at(id)).color;
            obj = static_cast<Primitive>(triangles.at(id));
        }
        else
        {
            normal = (hitpoint - dynamic_cast<Sphere&>(spheres.at(id)).center).Normalized();
            col = dynamic_cast<Sphere&>(spheres.at(id)).color;
            obj = static_cast<Primitive>(spheres.at(id));
        }

        Vector nl = normal;


        // Obtain flipped normal, if object hit from inside
        if (normal.Dot(_ray.dir) > 0)
            nl = nl * -1.0;

        // Maximum RGB reflectivity for Russian Roulette
        double p = col.Max();

        // After max bounces or if max reflectivity is zero...
        if (_depth > maxDepth || !p)
        {
            // Russian Roulette
            if (normRand() < p)            
                col = col * (1.0 / p);      // Scale estimator to remain unbiased 
            else
                return obj.emission * _E;   // No further bounces, only return potential emission
        }

        if (obj.refl == DIFF)
        {
            // Compute random reflection vector on hemisphere
            double r1 = 2.0 * M_PI * normRand();
            double r2 = normRand();
            double r2s = sqrt(r2);

            // Set up local orthogonal coordinate system u,v,w on surface
            Vector w = nl;
            Vector u;

            if (fabs(w.x) > .1)
                u = Vector(0.0, 1.0, 0.0);
            else
                u = (Vector(1.0, 0.0, 0.0).Cross(w)).Normalized();

            Vector v = w.Cross(u);

            // Random reflection vector d
            Vector d = (u * cos(r1) * r2s +
                        v * sin(r1) * r2s +
                        w * sqrt(1.0 - r2)).Normalized();

            // Explicit computation of direct lighting
            Vector e;

            //const Sphere &sphere = spheres.at(i);
            const Sphere& sphere = LightSource;

            if (sphere.emission.x <= 0 && sphere.emission.y <= 0 && sphere.emission.z <= 0)
            {
                std::cerr << "not light source ! " << std::endl;
            }

            // Randomly sample spherical light source from surface intersection

            // Set up local orthogonal coordinate system su,sv,sw towards light source
            Vector sw = sphere.center - hitpoint;
            Vector su;

            if (fabs(sw.x) > 0.1)
                su = Vector(0.0, 1.0, 0.0);
            else
                su = Vector(1.0, 0.0, 0.0);

            su = (su.Cross(/*w*/sw)).Normalized();
            Vector sv = sw.Cross(su);
            /*Vector sv = (sw.Cross(w)).Normalized();
            Vector su = (sv.Cross(sw)).Normalized();*/

            // Create random sample direction l towards spherical light source
            double cos_a_max = sqrt(1.0 - sphere.radius * sphere.radius /
                (hitpoint - sphere.center).Dot(hitpoint - sphere.center));
            double eps1 = normRand();
            double eps2 = normRand();
            double cos_a = 1.0 - eps1 + eps1 * cos_a_max;
            double sin_a = sqrt(1.0 - cos_a * cos_a);
            double phi = 2.0 * M_PI * eps2;
            Vector l = su * cos(phi) * sin_a +
                sv * sin(phi) * sin_a +
                sw * cos_a;
            l = l.Normalized();

            // Shoot shadow ray, check if intersection is with light source
            if ( IntersectLightSource(Ray(hitpoint, l)) )
            {
                //solid angle (on a unit sphere)
                double omega = 2 * M_PI * (1 - cos_a_max);

                // Add diffusely reflected light from light source; note constant BRDF 1/PI
                e = e + col.MultComponents(sphere.emission * l.Dot(nl) * omega) * M_1_PI;
            }
            

            // Return potential light emission, direct lighting, and indirect lighting 
            // (via recursive call for Monte-Carlo integration
            return obj.emission * _E + e + col.MultComponents(Radiance(Ray(hitpoint, d), _depth, 0));

        }
        else if (obj.refl == SPEC)
        {
            // Return light emission mirror reflection 
            // (via recursive call using perfect reflection vector)
            return obj.emission +
                col.MultComponents(Radiance(Ray(hitpoint, _ray.dir - normal * 2 * normal.Dot(_ray.dir)),
                    _depth, 1));
        }

        // Otherwise object transparent, i.e. assumed dielectric glass material
        Ray reflRay(hitpoint, _ray.dir - normal * 2 * normal.Dot(_ray.dir));  // Prefect reflection
        bool into = normal.Dot(nl) > 0;       // Bool for checking if ray from outside going in
        double nc = 1.0;                      // Index of refraction of air (approximately)
        double nt = 1.5;                      // Index of refraction of glass (approximately)
        double nnt = 0.0;

        // Set ratio depending on hit from inside or outside
        if (into)
            nnt = nc / nt;
        else
            nnt = nt / nc;

        double ddn = _ray.dir.Dot(nl);
        double cos2t = 1.0 - nnt * nnt * (1.0 - ddn * ddn);

        // Check for total internal reflection, if so only reflect
        if (cos2t <= 0.0)
        {
            // move reflection ray origin to the sphere surface
            Ray reflRay2(hitpoint + normal * dynamic_cast<Sphere&>(spheres.at(id)).radius, 
                         _ray.dir - normal * 2 * normal.Dot(_ray.dir));
            return obj.emission + col.MultComponents(Radiance(reflRay2, _depth, 1));
        }

        // Otherwise reflection and/or refraction occurs
        Vector tdir;

        // Determine transmitted ray direction for refraction
        if (into)
            tdir = (_ray.dir * nnt - normal * (ddn * nnt + sqrt(cos2t))).Normalized();
        else
            tdir = (_ray.dir * nnt + normal * (ddn * nnt + sqrt(cos2t))).Normalized();

        // Determine R0 for Schlick´s approximation
        double a = nt - nc;
        double b = nt + nc;
        double R0 = a * a / (b * b);

        // Cosine of correct angle depending on outside/inside
        double c;
        if (into)
            c = 1 + ddn;
        else
            c = 1 - tdir.Dot(normal);

        // Compute Schlick´s approximation of Fresnel equation
        double Re = R0 + (1 - R0) * c * c * c * c * c;   // Reflectance
        double Tr = 1 - Re;                              // Transmittance

        // Probability for selecting reflectance or transmittance
        double P = .25 + .5 * Re;
        double RP = Re / P;         // Scaling factors for unbiased estimator
        double TP = Tr / (1 - P);

        if (_depth < 3)   // Initially both reflection and transmission
            return obj.emission + col.MultComponents(Radiance(reflRay, _depth, 1) * Re +
                Radiance(Ray(hitpoint, tdir), _depth, 1) * Tr);
        else             // Russian Roulette
            if (normRand() < P)
                return obj.emission + col.MultComponents(Radiance(reflRay, _depth, 1) * RP);
            else
                return obj.emission + col.MultComponents(Radiance(Ray(hitpoint, tdir), _depth, 1) * TP);
    }


    /*
    * Main routine: Computation of path tracing image (2x2 subpixels)
    * Key parameters
    * - Image dimensions: width, height
    * - Number of samples per subpixel (non-uniform filtering): samples
    * Rendered result saved as PPM image file
    */
    int Render(Image _img)
    {
        // Set camera origin and viewing direction (negative z direction)
        Ray camera(Vector(50.0, 52.0, 295.6), Vector(0.0, -0.042612, -1.0).Normalized());

        // Image edge vectors for pixel sampling
        Vector cx = Vector(_img.width * 0.5135 / _img.height, 0.0, 0.0);
        Vector cy = (cx.Cross(camera.dir)).Normalized() * 0.5135;

        std::cout << "Starts rendering ... " << std::endl;

        // Loop over image rows
        #pragma omp parallel for
        for (int y = 0; y < _img.height; y++)
        {
            // init pseudo-random number generator
            srand(y * y * y);

            // Loop over row pixels
            #pragma omp parallel for
            for (int x = 0; x < _img.width; x++)
            {
                // init current pixel with black
                _img.setColor(x, y, Color());

                // 2x2 subsampling per pixel
                for (int sy = 0; sy < 2; sy++)
                {
                    for (int sx = 0; sx < 2; sx++)
                    {
                        Color accumulated_radiance;

                        // Compute radiance at subpixel using multiple samples
                        for (int s = 0; s < nbSamples; s++)
                        {
                            const double r1 = 2.0 * normRand();
                            const double r2 = 2.0 * normRand();

                            // Transform uniform into non-uniform filter samples
                            double dx;
                            if (r1 < 1.0)
                                dx = sqrt(r1) - 1.0;
                            else
                                dx = 1.0 - sqrt(2.0 - r1);

                            double dy;
                            if (r2 < 1.0)
                                dy = sqrt(r2) - 1.0;
                            else
                                dy = 1.0 - sqrt(2.0 - r2);

                            // Ray direction into scene from camera through sample
                            Vector dir = cx * ((x + (sx + 0.5 + dx) / 2.0) / _img.width - 0.5) +
                                         cy * ((y + (sy + 0.5 + dy) / 2.0) / _img.height - 0.5) +
                                         camera.dir;

                            // Extend camera ray to start inside box
                            Vector start = camera.org + dir * 130.0;

                            // build ray 
                            dir = dir.Normalized();
                            Ray ray(start, dir);

                            ThinLens(ray);

                            // Accumulate radiance
                            accumulated_radiance = accumulated_radiance + Radiance(ray, 0, 1) / (double)nbSamples;
                        }

                        accumulated_radiance = accumulated_radiance.clamp() * 0.25;

                        _img.addColor(x, y, accumulated_radiance);
                    }
                }
            }
        }
        std::cout << "Done! " << std::endl;   
    }

} //namespace pathTracing

int main(int argc, char* argv[])
{
    // Image to store final rendering
    const unsigned int width = 1024;
    const unsigned int height = 768;
    pathTracing::Image img(width, height);

    // Performs path tracing
    pathTracing::Render(img);

    // save image output
    img.Save(std::string("result.ppm"));

}