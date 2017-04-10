/*
  CSC418 - RayTracer code - Winter 2017 - Assignment 3&4
  Written Dec. 9 2010 - Jan 20, 2011 by F. J. Estrada
  Freely distributable for adacemic purposes only.
  Uses Tom F. El-Maraghi's code for computing inverse
  matrices. You will need to compile together with
  svdDynamic.c
  You need to understand the code provided in
  this file, the corresponding header file, and the
  utils.c and utils.h files. Do not worry about
  svdDynamic.c, we need it only to compute
  inverse matrices.
  You only need to modify or add code in sections
  clearly marked "TO DO"
*/

#include "utils.h"
#include "float.h"
#include "pthread.h" 

// A couple of global structures and data: An object list, a light list, and the
// maximum recursion depth
struct object3D *object_list;
struct pointLS *light_list;
struct colourRGB background;  
int MAX_DEPTH;
pthread_cond_t commonStart = PTHREAD_COND_INITIALIZER;
pthread_mutex_t commonStartLock = PTHREAD_MUTEX_INITIALIZER;

void buildScene(void)
{
 // Sets up all objects in the scene. This involves creating each object,
 // defining the transformations needed to shape and position it as
 // desired, specifying the reflectance properties (albedos and colours)
 // and setting up textures where needed.
 // Light sources must be defined, positioned, and their colour defined.
 // All objects must be inserted in the object_list. All light sources
 // must be inserted in the light_list.
 //
 // To create hierarchical objects:
 //   Copy the transform matrix from the parent node to the child, and
 //   apply any required transformations afterwards.
 //
 // NOTE: After setting up the transformations for each object, don't
 //       forget to set up the inverse transform matrix!

 struct object3D *o;
 struct pointLS *l;
 struct point3D p; // Background colour

 ///////////////////////////////////////
 // TO DO: For Assignment 3 you have to use
 //        the simple scene provided
 //        here, but for Assignment 4 you
 //        *MUST* define your own scene.
 //        Part of your mark will depend
 //        on how nice a scene you
 //        create. Use the simple scene
 //        provided as a sample of how to
 //        define and position objects.
 ///////////////////////////////////////

 // Simple scene for Assignment 3:
 // Insert a couple of objects. A plane and two spheres
 // with some transformations.

 // Let's add a plane
 // Note the parameters: ra, rd, rs, rg, R, G, B, alpha, r_index, and shinyness)
 o=newPlane(.05,.75,.05,.05,.55,.8,.75,1,1,2);  // Note the plane is highly-reflective (rs=rg=.75) so we
            // should see some reflections if all is done properly.
            // Colour is close to cyan, and currently the plane is
            // completely opaque (alpha=1). The refraction index is
            // meaningless since alpha=1
 Scale(o,6,6,1);        // Do a few transforms...
 RotateZ(o,PI/1.20);
 RotateX(o,PI/2.25);
 Translate(o,0,-3,10);
 invert(&o->T[0][0],&o->Tinv[0][0]);    // Very important! compute
           //and store the inverse
           // transform for this object!
 insertObject(o,&object_list);      // Insert into object list

 // Let's add a couple spheres
 o=newSphere(.05,.95,.35,.35,1,.25,.25,1,1,6);
 Scale(o,.75,.5,1.5);
 //Scale(o,1,1,1);
 RotateY(o,PI/2);
 Translate(o,-1.45,1.1,3.5);
 invert(&o->T[0][0],&o->Tinv[0][0]);
 insertObject(o,&object_list);

 o=newSphere(.05,.95,.95,.75,.75,.95,.55,1,1,6);
 Scale(o,.5,2.0,1.0);
 RotateZ(o,PI/1.5);
 Translate(o,1.75,1.25,5.0);
 invert(&o->T[0][0],&o->Tinv[0][0]);
 insertObject(o,&object_list);

 // Insert a single point light source.
 p.px=0;
 p.py=15.5;
 p.pz=-5.5;
 p.pw=1;
 l=newPLS(&p,.95,.95,.95);
 insertPLS(l,&light_list);

 // End of simple scene for Assignment 3
 // Keep in mind that you can define new types of objects such as cylinders and parametric surfaces,
 // or, you can create code to handle arbitrary triangles and then define objects as surface meshes.
 //
 // Remember: A lot of the quality of your scene will depend on how much care you have put into defining
 //           the relflectance properties of your objects, and the number and type of light sources
 //           in the scene.
}


void rtShade(struct object3D *obj, struct point3D *p, struct point3D *n, struct ray3D *ray, int depth, double a, double b, struct colourRGB *col)
{
 // This function implements the shading model as described in lecture. It takes
 // - A pointer to the first object intersected by the ray (to get the colour properties)
 // - The coordinates of the intersection point (in world coordinates)
 // - The normal at the point
 // - The ray (needed to determine the reflection direction to use for the global component, as well as for
 //   the Phong specular component)
 // - The current racursion depth
 // - The (a,b) texture coordinates (meaningless unless texture is enabled)
 //
 // Returns:
 // - The colour for this ray (using the col pointer)
 //
 struct colourRGB tmp_col, refl_col;  // Accumulator for colour components

 double R,G,B;      // Colour for the object in R G and B

 // This will hold the colour as we process all the components of
 // the Phong illumination model
 tmp_col.R=0;
 tmp_col.G=0;
 tmp_col.B=0;

 if (obj->texImg==NULL)   // Not textured, use object colour
 {
  R=obj->col.R;
  G=obj->col.G;
  B=obj->col.B;
 }
 else
 {
  // Get object colour from the texture given the texture coordinates (a,b), and the texturing function
  // for the object. Note that we will use textures also for Photon Mapping.
  obj->textureMap(obj->texImg,a,b,&R,&G,&B);
 }

 //////////////////////////////////////////////////////////////
 // TO DO: Implement this function. Refer to the notes for
 // details about the shading model.
 //////////////////////////////////////////////////////////////
// for light in scene create ray from intersection to light 


R = obj->col.R;
G = obj->col.G;
B = obj->col.B;
 
double shinyness = obj->shinyness;
double ra = obj->alb.ra;
double rd = obj->alb.rd;
double rs = obj->alb.rs;
double rg = obj->alb.rg;
 
double *lambda = (double *) malloc (sizeof (double));
struct object3D *objHit;
struct point3D *pHit = newPoint(0, 0, 0);
struct point3D *nHit = newPoint(0, 0, 0);

//point3D r = ray->d - 2 * dot(&ray->d , n)  * n;
struct point3D *r = newPoint(dot(&ray->d , n) * n->px, dot(&ray->d , n) * n->py, dot(&ray->d , n) * n->pz);
r->px = -r->px;
r->py = -r->py;
r->pz = -r->pz;
addVectors(r, r);
addVectors(&ray->d, r);
normalize(r);
struct ray3D *reflectedRay = newRay(p, r);

struct pointLS *currLight = light_list;
struct point3D *direction = newPoint(0, 0, 0);
  while (currLight != NULL)

  {    
    direction->px = currLight->p0.px;
    direction->py = currLight->p0.py;
    direction->pz = currLight->p0.pz;

    subVectors(p, direction);
    double intensityAmbient = 1/length(direction);
    normalize(direction);
    
    struct ray3D *shadowRay = newRay(p, direction);
  
    double intensityDiffuse = 1;
    double intensitySpecular = 1;

    findFirstHit(shadowRay, lambda, obj, &objHit, pHit, nHit, &a, &b);
    if(*lambda == DBL_MAX)
    {
      
   //fprintf(stderr,"ra: %f  rs: %f  rd: %f \n", ra, rs, rd);
     double ambient = ra * intensityAmbient;
     double specular = rs * pow(max(0, dot(&(shadowRay->d) , r)), shinyness) * intensitySpecular;
     double diffuse = rd * max(0, dot(n, &shadowRay->d)) * intensityDiffuse;
   
   //fprintf(stderr,"ambient diffuse spec %f %f %f\n", ambient, diffuse, specular);
   
     tmp_col.R +=   (ambient + diffuse + specular) * R;
     tmp_col.G +=   (ambient + diffuse + specular) * G;
     tmp_col.B +=  (ambient + diffuse + specular) * B;
   
   //fprintf(stderr,"R G B %f %f %f\n", tmp_col.R, tmp_col.G, tmp_col.B);
      
    
    }
      currLight = currLight->next;
 
    if (depth >= 0 && depth < MAX_DEPTH)
    {
      // Color passed in here is not correct. just there to compile for now. 
      rayTrace(reflectedRay, --depth, &refl_col, obj);

       tmp_col.R +=  refl_col.R;
       tmp_col.G +=  refl_col.G;
       tmp_col.B +=  refl_col.B;

    }
  }
    printf("%d fdsafhakhkjafdshklsahdf\n", depth);
  
   col->R = min(tmp_col.R, 1);// * rg;
   col->G = min(tmp_col.G, 1);// * rg;
   col->B = min(tmp_col.B, 1);//* rg;
 // Be sure to update 'col' with the final colour computed here!
 free(reflectedRay);
 free(lambda);
 free(nHit);
 free(pHit);
 

}

void findFirstHit(struct ray3D *ray, double *lambda, struct object3D *Os, struct object3D **obj, struct point3D *p, struct point3D *n, double *a, double *b)
{
 // Find the closest intersection between the ray and any objects in the scene.
 // It returns:
 //   - The lambda at the intersection (or < 0 if no intersection)
 //   - The pointer to the object at the intersection (so we can evaluate the colour in the shading function)
 //   - The location of the intersection point (in p)
 //   - The normal at the intersection point (in n)
 //
 // Os is the 'source' object for the ray we are processing, can be NULL, and is used to ensure we don't 
 // return a self-intersection due to numerical errors for recursive raytrace calls.
 //

 /////////////////////////////////////////////////////////////
 // TO DO: Implement this function. See the notes for
 // reference of what to do in here
 /////////////////////////////////////////////////////////////
 
 struct object3D* curr_object = object_list;
 double closest, curr_distance, curr_lambda;
 struct point3D *closest_p, *closest_n;
 struct point3D *curr_p, *curr_n;
 struct point3D distance_test;

 curr_p = newPoint(0, 0, 0);
 curr_n = newPoint(0, 0, 0); 
 closest_p = newPoint(0, 0, 0);
 closest_n = newPoint(0, 0, 0);

 closest = DBL_MAX;
 while (curr_object != NULL)
 {
   if (curr_object != Os)
   {
    curr_object->intersect(curr_object, ray, &curr_lambda, curr_p, curr_n, a, b);


    //fprintf(stderr, "%f\n", lambda);
    //fprintf(stderr,"%f %f %f\n",curr_object->col.R,curr_object->col.G,curr_object->col.B);
    if (curr_lambda < DBL_MAX )
    { 
      distance_test = ray->p0;
      subVectors(curr_p, &distance_test);
      curr_distance = length(&distance_test);
      if (curr_distance < closest)
      {

      *obj = curr_object;
      closest = curr_distance;
      memcpy(closest_n,curr_n,sizeof(point3D));
      memcpy(closest_p,curr_p,sizeof(point3D));
      }
    }
    //first_hit = &curr_object;
    
  }
  curr_object = curr_object->next;
 }
 
 *lambda = closest;
 memcpy(n,closest_n,sizeof(point3D));
 memcpy(p,closest_p,sizeof(point3D));   

 free(closest_n);
 free(closest_p);
 free(curr_n);
 free(curr_p);
 /////////////////////////////////////////////////////////////
 // TO DO: Implement this function. See the notes for
 // reference of what to do in here
 /////////////////////////////////////////////////////////////

}
void *rayTraceT(void *params)
{

  pthread_cond_wait(&commonStart, &commonStartLock);
  pthread_mutex_unlock(&commonStartLock);
  rayTraceParams i_params;
  struct colourRGB col;
  rayTrace(i_params.ray, i_params.depth, &col, i_params.Os);

  free(i_params.ray);
  *(i_params.rgbIm) = (unsigned char)(col.R *255);
  *(i_params.rgbIm+1) = (unsigned char)(col.G*255);
  *(i_params.rgbIm+2) = (unsigned char)(col.B*255);
  printf("%d a\n", i_params.id);
  pthread_exit(NULL);
}

void rayTrace(struct ray3D *ray, int depth, struct colourRGB *col, struct object3D *Os)
{
 // Ray-Tracing function. It finds the closest intersection between
 // the ray and any scene objects, calls the shading function to
 // determine the colour at this intersection, and returns the
 // colour.
 //
 // Os is needed for recursive calls to ensure that findFirstHit will
 // not simply return a self-intersection due to numerical
 // errors. For the top level call, Os should be NULL. And thereafter
 // it will correspond to the object from which the recursive
 // ray originates.
 //

 double *lambda;   // Lambda at intersection
 double a,b;    // Texture coordinates
 struct object3D *obj;  // Pointer to object at intersection
 struct point3D *p;  // Intersection point
 struct point3D *n;  // Normal at intersection
 struct colourRGB I;  // Colour returned by shading function


 
 lambda =  (double *) malloc (sizeof (double));

 p = newPoint(0, 0, 0);
 n = newPoint(0, 0, 0);
 // findFirstHit to get the object it hits and rtShade 
 findFirstHit(ray, lambda, Os, &obj, p, n, &a, &b);
 // n->px = -n->px;
 // n->py = -n->py;
 // n->pz = -n->pz;
    

 if(*lambda < DBL_MAX){
 //fprintf(stderr,"%.4f %.4f %.4f %.4f %.4f %.4f\n",  p->px, p->py, p->pz, n->px, n->py, n->pz);
  I.R = obj->col.R;
  I.G = obj->col.G;
  I.B = obj->col.B;

   rtShade(obj, p, n, ray, depth, a, b, &I);
 }
 else
 {
   I.R = background.R;
   I.G = background.G;
   I.B = background.B;
 }
 memcpy(col, &I, sizeof(colourRGB));
 free(n);
 free(p);
 free(lambda);

  // memcpy(col, &I, sizeof(colourRGB));
 ///////////////////////////////////////////////////////
 // TO DO: Complete this function. Refer to the notes
 // if you are unsure what to do here.
 ///////////////////////////////////////////////////////
}

int main(int argc, char *argv[])
{
 // Main function for the raytracer. Parses input parameters,
 // sets up the initial blank image, and calls the functions
 // that set up the scene and do the raytracing.
 struct image *im;  // Will hold the raytraced image
 struct view *cam;  // Camera and view for this scene
 int sx;    // Size of the raytraced image
 int antialiasing;  // Flag to determine whether antialiaing is enabled or disabled
 char output_name[1024];  // Name of the output file for the raytraced .ppm image
 struct point3D e;    // Camera view parameters 'e', 'g', and 'up'
 struct point3D g;
 struct point3D up;
 double du, dv;     // Increase along u and v directions for pixel coordinates
 struct point3D pc,d;   // Point structures to keep the coordinates of a pixel and
        // the direction or a ray
 struct ray3D *ray;   // Structure to keep the ray from e to a pixel
 struct colourRGB col;    // Return colour for raytraced pixels
 int i,j;     // Counters for pixel coordinates
 unsigned char *rgbIm;
 pthread_t *pixels;

 if (argc<5)
 {
  fprintf(stderr,"RayTracer: Can not parse input parameters\n");
  fprintf(stderr,"USAGE: RayTracer size rec_depth antialias output_name\n");
  fprintf(stderr,"   size = Image size (both along x and y)\n");
  fprintf(stderr,"   rec_depth = Recursion depth\n");
  fprintf(stderr,"   antialias = A single digit, 0 disables antialiasing. Anything else enables antialiasing\n");
  fprintf(stderr,"   output_name = Name of the output file, e.g. MyRender.ppm\n");
  exit(0);
 }
 sx=atoi(argv[1]);
 MAX_DEPTH=atoi(argv[2]);
 if (atoi(argv[3])==0) antialiasing=0; else antialiasing=1;
 strcpy(&output_name[0],argv[4]);

 fprintf(stderr,"Rendering image at %d x %d\n",sx,sx);
 fprintf(stderr,"Recursion depth = %d\n",MAX_DEPTH);
 if (!antialiasing) fprintf(stderr,"Antialising is off\n");
 else fprintf(stderr,"Antialising is on\n");
 fprintf(stderr,"Output file name: %s\n",output_name);

 object_list=NULL;
 light_list=NULL;

 // Allocate memory for the new image
 im=newImage(sx, sx);
 if (!im)
 {
  fprintf(stderr,"Unable to allocate memory for raytraced image\n");
  exit(0);
 }
 else rgbIm=(unsigned char *)im->rgbdata;
 pthread_cond_init(&commonStart, NULL);
 pthread_mutex_init(&commonStartLock, NULL);
 pixels = new pthread_t[sx*sx];

 ///////////////////////////////////////////////////
 // TO DO: You will need to implement several of the
 //        functions below. For Assignment 3, you can use
 //        the simple scene already provided. But
 //        for Assignment 4 you need to create your own
 //        *interesting* scene.
 ///////////////////////////////////////////////////
 buildScene();    // Create a scene. This defines all the
      // objects in the world of the raytracer

 //////////////////////////////////////////
 // TO DO: For Assignment 3 you can use the setup
 //        already provided here. For Assignment 4
 //        you may want to move the camera
 //        and change the view parameters
 //        to suit your scene.
 //////////////////////////////////////////

 // Mind the homogeneous coordinate w of all vectors below. DO NOT
 // forget to set it to 1, or you'll get junk out of the
 // geometric transformations later on.

 // Camera center is at (0,0,-1)
 e.px=0;
 e.py=0;
 e.pz=-3;
 e.pw=1;

 // To define the gaze vector, we choose a point 'pc' in the scene that
 // the camera is looking at, and do the vector subtraction pc-e.
 // Here we set up the camera to be looking at the origin, so g=(0,0,0)-(0,0,-1)
 g.px=0;
 g.py=0;
 g.pz=1;
 g.pw=0;

 // Define the 'up' vector to be the Y axis
 up.px=0;
 up.py=1;
 up.pz=0;
 up.pw=0;

 // Set up view with given the above vectors, a 4x4 window,
 // and a focal length of -1 (why? where is the image plane?)
 // Note that the top-left corner of the window is at (-2, 2)
 // in camera coordinates.
 cam=setupView(&e, &g, &up, -3, -2, 2, 4);

 if (cam==NULL)
 {
  fprintf(stderr,"Unable to set up the view and camera parameters. Our of memory!\n");
  cleanup(object_list,light_list);
  deleteImage(im);
  exit(0);
 }

 // Set up background colour here
 background.R=0.2;
 background.G=0.2;
 background.B=0.2;

 // Do the raytracing
 //////////////////////////////////////////////////////
 // TO DO: You will need code here to do the raytracing
 //        for each pixel in the image. Refer to the
 //        lecture notes, in particular, to the
 //        raytracing pseudocode, for details on what
 //        to do here. Make sure you undersand the
 //        overall procedure of raytracing for a single
 //        pixel.
 //////////////////////////////////////////////////////
 du=cam->wsize/(sx-1);    // du and dv. In the notes in terms of wl and wr, wt and wb,
 dv=-cam->wsize/(sx-1);   // here we use wl, wt, and wsize. du=dv since the image is
        // and dv is negative since y increases downward in pixel
        // coordinates and upward in camera coordinates.

 fprintf(stderr,"View parameters:\n");
 fprintf(stderr,"Left=%f, Top=%f, Width=%f, f=%f\n",cam->wl,cam->wt,cam->wsize,cam->f);
 fprintf(stderr,"Camera to world conversion matrix (make sure it makes sense!):\n");
 printmatrix(cam->C2W);
 fprintf(stderr,"World to camera conversion matrix\n");
 printmatrix(cam->W2C);
 fprintf(stderr,"\n");

  fprintf(stderr,"Rendering row: ");
  //since the origin at 0, can easily find direction without subtracting origin from image point.


 for (j=0;j<sx;j++)   // For each of the pixels in the image
 {
  fprintf(stderr,"%d/%d, ",j,sx);
  for (i=0;i<sx;i++)
  {
    pc.px = 0;
    pc.py = 0;
    pc.pz = 0;
    pc.pw = 1;
    d.px = (-sx/2 + (i + 0/5))*du;
    d.py = (-sx/2 + (j + 0/5))*dv;
    d.pz = -1;
    d.pw = 0;

    normalize(&d);
    //fprintf(stderr,"%.4f %.4f %.4f ", d.px, d.py, d.pz); c
  
    matVecMult(cam->C2W, &d);
    matVecMult(cam->C2W, &pc);
    //printf("%.4f %.4f %.4f \n", d.px, d.py, d.pz); //current camera points to positive z

   
    ////////////////////////////////////////////////////s///////////////
    // TO DO - complete the code that should be in this loop to do the
    //         raytracing!
    ///////////////////////////////////////////////////////////////////
    ray = newRay(&pc, &d);
    rayTraceParams params;
    params.ray = ray;
    params.depth = MAX_DEPTH;
    params.Os = NULL;
    params.rgbIm = rgbIm+3*(sx*j+i);
    params.id = sx*j+i;

    printf("%d hjk\n", (params.id));

    pthread_create(&pixels[j*sx+i], NULL, rayTraceT, &params);
  } 

  // end for i
 } 

 pthread_cond_broadcast(&commonStart);
 for (j=0;j<sx;j++)   // For each of the pixels in the image
 {
  for (i=0;i<sx;i++)
  {

    pthread_join(pixels[j*sx+i], NULL);
    // free(ray);
    // *(rgbIm+3*(sx*j+i)) = (unsigned char)(col.R *255);
    // *(rgbIm+(3*(sx*j+i)+1)) = (unsigned char)(col.G*255);
    // *(rgbIm+(3*(sx*j+i)+2)) = (unsigned char)(col.B*255);
  } 

  // end for i
 } // end for j

 fprintf(stderr,"\nDone!\n");

 // Output rendered image
 imageOutput(im,output_name);

 // Exit section. Clean up and return.
 cleanup(object_list,light_list);   // Object and light lists
 deleteImage(im);       // Rendered image
 free(cam);         // camera view
 exit(0);
}