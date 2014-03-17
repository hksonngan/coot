/*
     pygl/cprimitive.h: CCP4MG Molecular Graphics Program
     Copyright (C) 2001-2008 University of York, CCLRC
     Copyright (C) 2009-2010 University of York
     Copyright (C) 2012 STFC

     This library is free software: you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public License
     version 3, modified in accordance with the provisions of the 
     license to address the requirements of UK law.
 
     You should have received a copy of the modified GNU Lesser General 
     Public License along with this library.  If not, copies may be 
     downloaded from http://www.ccp4.ac.uk/ccp4license.php
 
     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU Lesser General Public License for more details.
*/


#ifndef _CCP4MG_PRIM_
#define _CCP4MG_PRIM_
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include "quat.h"
#include "cartesian.h"
#include "volume.h"
#include "ppmutil.h"
#include "font_info.h"

#ifdef __APPLE_CC__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#if defined (_WIN32) && not defined (WINDOWS_MINGW)
#define EXAMPLE_DLL __declspec(dllimport)
void __stdcall EXAMPLE_DLL draw_flat_ribbon(const std::vector<Cartesian> &spline, const std::vector<Cartesian> &pv, const std::vector<Cartesian> &pvpr, int npoints, int textured, int multicolour, const std::vector<Cartesian> &colour_vector, const bool two_colour=false, const bool grey_ribbon_edge=false);
void __stdcall EXAMPLE_DLL draw_flat_rounded_ribbon(const std::vector<Cartesian> &spline, const std::vector<Cartesian> &pv, const std::vector<Cartesian> &pvpr, int npoints, int textured, int multicolour, const std::vector<Cartesian> &colour_vector, const bool two_colour=false);
void __stdcall EXAMPLE_DLL draw_fancy_ribbon(const std::vector<Cartesian> &spline, const std::vector<Cartesian> &pv, const std::vector<Cartesian> &pvpr, int npoints, int textured, int multicolour, const std::vector<Cartesian> &colour_vector, const bool two_colour=false);
void __stdcall EXAMPLE_DLL draw_elliptical_ribbon(const std::vector<Cartesian> &vertices, const std::vector<Cartesian> &pv, const std::vector<Cartesian> &pvpr, int quality, int textured, int multicolour, const std::vector<Cartesian> &colour_vector, const bool two_colour=false);
#else
void draw_flat_ribbon(const std::vector<Cartesian> &spline, const std::vector<Cartesian> &pv, const std::vector<Cartesian> &pvpr, int npoints, int textured, int multicolour, const std::vector<Cartesian> &colour_vector, const bool two_colour=false, const bool grey_ribbon_edge=false);
void draw_flat_rounded_ribbon(const std::vector<Cartesian> &spline, const std::vector<Cartesian> &pv, const std::vector<Cartesian> &pvpr, int npoints, int textured, int multicolour, const std::vector<Cartesian> &colour_vector, const bool two_colour=false);
void draw_fancy_ribbon(const std::vector<Cartesian> &spline, const std::vector<Cartesian> &pv, const std::vector<Cartesian> &pvpr, int npoints, int textured, int multicolour, const std::vector<Cartesian> &colour_vector, const bool two_colour=false);
void draw_elliptical_ribbon(const std::vector<Cartesian> &vertices, const std::vector<Cartesian> &pv, const std::vector<Cartesian> &pvpr, int quality, int textured, int multicolour, const std::vector<Cartesian> &colour_vector, const bool two_colour=false);
#endif
void draw_billboard(double v11, double v12, double v21, double v22);
void draw_quadstrip_outline(const double *vertices, int nvertices);
void draw_quadstrip_textured(const double *vertices, int nvertices, int textured);
void set_line_colour(double r, double g, double b, double a);
void draw_line(double v11, double v12, double v13, double v21, double v22, double v23, double size);
void draw_sphere(double v1, double v2, double v3, int accu, double size);

void draw_cylinder(const std::vector<Cartesian> &vertices, double size,int accu, int textured, bool force_dl=false, bool capped=false);
void draw_capped_cylinder(const std::vector<Cartesian> &vertices, double size,int accu, int textured, bool force_dl=false);

void ForceLoadTextures();
void UnForceLoadTextures();

struct SVertex
{
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat nx;
    GLfloat ny;
    GLfloat nz;
    GLubyte r;
    GLubyte g;
    GLubyte b;
    GLubyte a;
    char padding[4];
};

class RenderQuality {
  static int render_quality;
  static int ribbon_quality;
 public:
  static void SetRenderQuality(int render_quality_in){render_quality = render_quality_in; }
  static int GetRenderQuality(){ return render_quality ; }
  static void SetSmoothRibbons(int ribbon_quality_in){ribbon_quality = ribbon_quality_in; }
  static int GetSmoothRibbons(){ return ribbon_quality ; }
};

class Lighting{
 public:
  Lighting();
  ~Lighting(){};
  GLfloat ambient[3];
  GLfloat diffuse[3];
  GLfloat specular[3];
  GLfloat emission[3];
  GLfloat shininess;
  void copy(const Lighting &light_in);
};

class Primitive{
 protected:

  GLuint cardVertexBuffer;
  GLuint cardOriginBuffer;
  GLuint cardSizeBuffer;
  GLuint cardNormalBuffer;
  GLuint cardColorBuffer;
  GLuint cardIndexBuffer;
  GLfloat *bufferVertices;
  GLfloat *bufferOrigins;
  GLfloat *bufferSizes;
  GLfloat *bufferNormals;
  GLfloat *bufferColors; 
  GLuint  *bufferIndices;

  GLuint  allBuffer;
  SVertex *bufferVNC;

  bool useInterleaved;

  int arraysGenerated;
  int arraysUploaded;
  int nVerts;
  int nRealVerts;
  GLenum polygonType;
  void generateArrays();
  void bindArrays();
  void blatBuffer(int arrayType, GLuint &bufferIndex, int dataSize, void ** data, int type);
  void freeResources();
  virtual void drawArrays(const double *override_colour=0, int selective_override=0);
  virtual std::vector<Primitive*> GetSimplePrimitivesArrays(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1);

  double colour[3];
  std::vector<Cartesian> colours;
  std::vector<Cartesian> vertices;
  std::vector<Cartesian> normals;
  int textured;
  int multitexture;
  double alpha;
  double size;
  Quat quat;
  Cartesian origin;
  int transparent;
  int colour_override;
  void OutputPovrayPigment(std::ofstream &fp);
  int occDataAttrib;
  int useVBO;
  int useVertexArrays;
 public:
  Primitive();
  virtual ~Primitive();
  const Cartesian& get_origin(void) const { return origin;};
  void set_origin(const Cartesian &origin_in);
  void set_origin(const double *origin_in);
  void SetColourOverride(int colour_override_in){colour_override = colour_override_in;};
  int GetColourOverride(void) const { return colour_override;};
  virtual void draw(const double *override_colour=0, int selective_override=0) = 0 ;
  virtual void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  virtual void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  virtual void set_draw_colour(const GLfloat *colp=0) = 0;
  virtual void SetAlpha(double alpha_in){ alpha = alpha_in; } ;
  double GetAlpha() const { return alpha; } ;
  void set_draw_colour_poly(void) ;
  void set_draw_colour_line(void) ;
  void set_draw_colour_poly_override(const GLfloat *colp) ;
  void set_draw_colour_line_override(const GLfloat *colp) ;
  void initialize_lighting(void);
  void apply_textures(void);
  void unbind_textures(void);
  int get_transparent(void) const {return transparent;};
  virtual void set_transparent(int trans_in){transparent=trans_in;};
  void SetColours(const std::vector<Cartesian> &colours_in){colours = colours_in;};
  const std::vector<Cartesian> &GetColours(void) const {return colours;};
  void SetNormals(const std::vector<Cartesian> &normals_in){normals = normals_in;};
  const std::vector<Cartesian> &GetNormals(void) const {return normals;};
  void SetVertices(const std::vector<Cartesian> &vertices_in){vertices = vertices_in;};
  const std::vector<Cartesian> &GetVertices(void) const {return vertices;};
  const double* GetColour(void) const {return colour;};
  double GetSize(void) const {return size;};
  void SetSize(double size_in){size=size_in;};
  void SetOrigin(const Cartesian &o_in) { origin = o_in;};
  void SetColour(const double *c_in) { colour[0] = c_in[0];colour[1] = c_in[1];colour[2] = c_in[2];};
  void SetColour(const std::vector<double> &c_in) { colour[0] = c_in[0];colour[1] = c_in[1];colour[2] = c_in[2];};
  virtual bool isLine() const {return false;};
  virtual bool isDots() const {return false;};
  virtual bool isImposter() const {return false;};
  virtual std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
  virtual size_t GetNumberOfSimplePrimitives() const { return 1;} ;
  void SetOccDataAttrib(int occDataAttrib_in) {occDataAttrib=occDataAttrib_in;} ;
  virtual void forceRegenerateArrays() {};
  int GetUseVBO() const {return useVBO;} ; 
  virtual void SetUseVBO(const int _useVBO); 
  int GetUseVertexArrays() const {return useVertexArrays;} ; 
  virtual void SetUseVertexArrays(const int _useVertexArrays); 
};

class BillboardPrimitive : public Primitive{
  float x_texture_start;
  float x_texture_end;
  float y_texture_start;
  float y_texture_end;
 public:
  BillboardPrimitive();
  virtual ~BillboardPrimitive();
  virtual void draw(double y, const Cartesian& x_trans, const Cartesian& y_trans, const Cartesian& z_trans) = 0;
  float GetTextureEndXCoord() const {return x_texture_end;};
  float GetTextureStartXCoord() const {return x_texture_start;};
  void SetTextureEndXCoord(float _in) {x_texture_end = _in;};
  void SetTextureStartXCoord(float _in) {x_texture_start = _in;};
  float GetTextureEndYCoord() const {return y_texture_end;};
  float GetTextureStartYCoord() const {return y_texture_start;};
  void SetTextureEndYCoord(float _in) {y_texture_end = _in;};
  void SetTextureStartYCoord(float _in) {y_texture_start = _in;};
  
};

class Point : public Primitive{
 public:
  Point();
  Point(const Cartesian &vertex, const double *colour, const Cartesian &origin, double size_in=1.0, double alpha_in=1.0);
  ~Point();
  void set_draw_colour(const GLfloat *col=0); 
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  bool isLine() const {return true;};
  bool isDots() const {return true;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
};

class PointElement : public Point {
 public:
  PointElement();
  ~PointElement();
  PointElement(const Cartesian &vertex, const double *colour, const Cartesian &origin, double size_in=1.0, double alpha_in=1.0);
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return true;};
};

class Line : public Primitive{
 public:
  Line();
  Line(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0);
  ~Line();
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  bool isLine() const {return true;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
};

class LineElement : public Line {
 public:
  LineElement();
  ~LineElement();
  LineElement(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0);
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return true;};
};

class ArrowElement : public LineElement {
 protected:
  int arrow_head;
  double arrowWidth, arrowLength;
 public:
  ArrowElement();
  ArrowElement(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0,int arrow_head=0);
  ~ArrowElement();
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  bool isLine() const {return true;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
  int GetArrowHead() const { return arrow_head; };
  void SetArrowHead(int arrow_head_in) { arrow_head = arrow_head_in; };
  void setArrowLength(double length) { arrowLength = length; };
  double getArrowLength() const { return arrowLength; };
  void setArrowWidth(double Width) { arrowWidth = Width; };
  double getArrowWidth() const { return arrowWidth; };
};

class Arrow : public ArrowElement {
 public:
  Arrow();
  Arrow(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0,int arrow_head=0);
  ~Arrow();
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  bool isLine() const {return true;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
};

class DashArrowElement : public LineElement  {
  double arrowWidth, arrowLength;
 protected:
  int arrow_head;
  double dash_length;
 public:
  DashArrowElement();
  DashArrowElement(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0,int arrow_head=0);
  ~DashArrowElement();
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  bool isLine() const {return true;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
  void SetDashLength(double length) { dash_length = length; };
  double GetDashLength() const { return dash_length; };
  int GetArrowHead() const { return arrow_head; };
  void SetArrowHead(int arrow_head_in) { arrow_head = arrow_head_in; };
  void setArrowLength(double length) { arrowLength = length; };
  double getArrowLength() const { return arrowLength; };
  void setArrowWidth(double Width) { arrowWidth = Width; };
  double getArrowWidth() const { return arrowWidth; };

};

class DashArrow : public DashArrowElement {
 public:
  DashArrow();
  DashArrow(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0,int arrow_head=0);
  ~DashArrow();
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;

};

class LineStrip : public Primitive{
 public:
  LineStrip();
  LineStrip(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0);
  ~LineStrip();
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  bool isLine() const {return true;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
};

class DashLine : public Primitive{
  double arrowWidth, arrowLength;
 public:
  DashLine();
  DashLine(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0);
  ~DashLine();
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  bool isLine() const {return true;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
  void SetDashLength(double length) { dash_length = length; };
  double GetDashLength() const { return dash_length; };
 private:
  double dash_length;

};

class CircleElement : public Primitive{
   matrix orientation;
 public:
  CircleElement();
  CircleElement(const Cartesian &vertex, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0, int textured_in=0);
  ~CircleElement();
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  bool isLine() const {return false;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
  void setMatrix(const matrix &a){orientation = a;} ;
  matrix getMatrix() const {return orientation;};
};

class PartialCircleElement : public CircleElement {
   matrix orientation;
   double startAngle;
   double sweepAngle;
 public:
  PartialCircleElement();
  PartialCircleElement(const Cartesian &vertex, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0, int textured_in=0, double startAngle_in=0.0, double sweepAngle_in=360.0);
  ~PartialCircleElement();
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return false;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
  void setMatrix(const matrix &a){orientation = a;} ;
  matrix getMatrix() const {return orientation;};
  void setStartAngle(double s){startAngle = s;} ;
  double getStartAngle() const {return startAngle;};
  void setSweepAngle(double s){sweepAngle = s;} ;
  double getSweepAngle() const {return sweepAngle;};
};

class BillboardCylinderElement : public BillboardPrimitive {
  public:
  BillboardCylinderElement();
  BillboardCylinderElement(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0, int textured_in=0);
  ~BillboardCylinderElement();
  void draw(double y, const Cartesian& x_trans, const Cartesian& y_trans, const Cartesian& z_trans);
  void draw(const double *override_colour=0, int selective_override=0);
  void set_draw_colour(const GLfloat *col=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v) {};
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v) {};
  bool isLine() const {return false;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
};

class BillboardCylinder : public BillboardCylinderElement{
 public:
  BillboardCylinder();
  BillboardCylinder(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0, int textured_in=0);
  ~BillboardCylinder();
  void draw(double y, const Cartesian& x_trans, const Cartesian& y_trans, const Cartesian& z_trans);
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return false;};
};

class ImposterSphereElement : public BillboardPrimitive {
  private:
    std::string text;
  public:
  ImposterSphereElement();
  ImposterSphereElement(const Cartesian &vertex, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0, int textured_in=0);
  ~ImposterSphereElement();
  void draw(double y, const Cartesian& x_trans, const Cartesian& y_trans, const Cartesian& z_trans);
  void draw(const double *override_colour=0, int selective_override=0);
  void set_draw_colour(const GLfloat *col=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v) {};
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v) {};
  bool isLine() const {return false;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
};

class ImposterSphere : public ImposterSphereElement {
 public:
  ImposterSphere();
  ImposterSphere(const Cartesian &vertex, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0, int textured_in=0);
  ~ImposterSphere();
  void draw(double y, const Cartesian& x_trans, const Cartesian& y_trans, const Cartesian& z_trans);
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return false;};
};


class BillboardLabelledCircleElement : public BillboardPrimitive {
  private:
    std::string text;
  public:
  BillboardLabelledCircleElement();
  BillboardLabelledCircleElement(const Cartesian &vertex, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0, int textured_in=0, const std::string text="");
  ~BillboardLabelledCircleElement();
  void draw(double y, const Cartesian& x_trans, const Cartesian& y_trans, const Cartesian& z_trans);
  void draw(const double *override_colour=0, int selective_override=0);
  void set_draw_colour(const GLfloat *col=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v) {};
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v) {};
  bool isLine() const {return false;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
  std::string GetText(void) const {return text;} ;
  void SetText(const std::string&t) {text=t;} ;
};

class BillboardLabelledCircle : public BillboardLabelledCircleElement {
 public:
  BillboardLabelledCircle();
  BillboardLabelledCircle(const Cartesian &vertex, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0, int textured_in=0, const std::string text_in="");
  ~BillboardLabelledCircle();
  void draw(double y, const Cartesian& x_trans, const Cartesian& y_trans, const Cartesian& z_trans);
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return false;};
};

class Circle : public CircleElement {
 public:
  Circle();
  Circle(const Cartesian &vertex, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0, int textured_in=0);
  ~Circle();
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return false;};
};

class PartialCircle : public PartialCircleElement {
 public:
  PartialCircle();
  PartialCircle(const Cartesian &vertex, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0, int textured_in=0, double startAngle_in=0.0, double sweepAngle_in=360.0);
  ~PartialCircle();
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return false;};
};

class TriangleElement: public Primitive{
 public:
  TriangleElement();
  TriangleElement(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in,  double alpha_in=1.0, int textured_in=0);
  ~TriangleElement();
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  bool isLine() const {return false;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
};

class PolygonElement: public Primitive{
 public:
  PolygonElement();
  PolygonElement(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in,  double alpha_in=1.0, int textured_in=0);
  ~PolygonElement();
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  bool isLine() const {return false;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
};

class QuadElement: public Primitive{
 public:
  QuadElement();
  QuadElement(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in,  double alpha_in=1.0, int textured_in=0);
  ~QuadElement();
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  bool isLine() const {return false;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
};

class SphereElement: public Primitive{
  int accu;
 public:
  SphereElement();
  SphereElement(const Cartesian &vertex, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0, int accu_in=0, int textured_in=0);
  ~SphereElement();
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  bool isLine() const {return false;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
};

class SpheroidElement: public Primitive{
  int accu;
  double a;
  double b;
  double c;
  Cartesian xaxis;
  Cartesian yaxis;
  Cartesian zaxis;
  int show_axes;
  int show_solid;
  matrix mat;
 public:
  SpheroidElement();
  SpheroidElement(const Cartesian &vertex, const double *colour_in, const Cartesian &origin_in, double a_in=1.0, double b_in=1.0, double c_in=1.0, const Cartesian& xaxis_in=Cartesian(1,0,0), const Cartesian& yaxis_in=Cartesian(0,1,0), const Cartesian& zaxis_in=Cartesian(0,0,1), double alpha_in=1.0, int accu_in=0, int show_axes=0, int show_solid=1, int textured_in=0);
  SpheroidElement(const Cartesian &vertex, const double *colour_in, const Cartesian &origin_in, const matrix &mat_in, double alpha_in=1.0, int accu_in=0, int show_axes_in=0, int show_solid_in=0, int textured_in=0);
  ~SpheroidElement();
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  void ShowSolid();
  void HideSolid();
  void ShowAxes();
  void HideAxes();
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  bool isLine() const {return false;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
  double GetA() const { return a;}
  double GetB() const { return b;}
  double GetC() const { return c;}
  Cartesian GetXAxis() const { return xaxis;}
  Cartesian GetYAxis() const { return yaxis;}
  Cartesian GetZAxis() const { return zaxis;}
  void SetA(const double a_in) { a=a_in;}
  void SetB(const double b_in) { b=b_in;}
  void SetC(const double c_in) { c=c_in;}
  void SetXAxis(const Cartesian &xaxis_in) { xaxis=xaxis_in;}
  void SetYAxis(const Cartesian &yaxis_in) { yaxis=yaxis_in;}
  void SetZAxis(const Cartesian &zaxis_in) { zaxis=zaxis_in;}
  void SetMatrix(const matrix &mat_in) { mat=mat_in;}
  matrix GetMatrix() const { return mat;}
  int GetDrawAxes() const { return show_axes; }
  int GetDrawSolid() const { return show_solid; }

};

class CylinderElement: public Primitive{
 protected:
  int accu;
 public:
  CylinderElement();
  CylinderElement(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0, int accu_in=8, int textured_in=0);
  ~CylinderElement();
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  bool isLine() const {return false;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
};

class DashCylinderElement: public CylinderElement {
 public:
  DashCylinderElement();
  DashCylinderElement(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0, int accu_in=8, int textured_in=0);
  ~DashCylinderElement();
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  bool isLine() const {return false;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
  void SetDashLength(double length) { dash_length = length; };
  double GetDashLength() const { return dash_length; };
  void SetDashEnd(int end) { dash_end = end; };
  int GetDashEnd() { return dash_end; };
 private:
  double dash_length;
  int dash_end;

};

class ConeElement: public CylinderElement{
 public:
  ConeElement();
  ConeElement(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0, int accu_in=8, int textured_in=0);
  ~ConeElement();
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  bool isLine() const {return false;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
};

class QuadStripElement : public Primitive{
 public:
  QuadStripElement();
  QuadStripElement(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in,  double alpha_in=1.0, int textured_in=0);
  ~QuadStripElement();
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  bool isLine() const {return false;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
  size_t GetNumberOfSimplePrimitives() const { if(vertices.size()>1) {return vertices.size()-2;} else {return 0;};} ;
};

class QuadStrip: public QuadStripElement {
 public:
  QuadStrip();
  ~QuadStrip(){};
  QuadStrip(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in,  double alpha_in=1.0, int textured_in=0);
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return false;};
};

class TriangleStripElement : public Primitive{
 public:
  TriangleStripElement();
  TriangleStripElement(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in,  double alpha_in=1.0, int textured_in=0);
  ~TriangleStripElement();
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  bool isLine() const {return false;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
  size_t GetNumberOfSimplePrimitives() const { if(vertices.size()>2) {return vertices.size()-3;} else {return 0;};} ;
};

class TriangleStrip: public TriangleStripElement {
 public:
  TriangleStrip();
  ~TriangleStrip(){};
  TriangleStrip(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in,  double alpha_in=1.0, int textured_in=0);
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return false;};
};


class TriangleFanElement : public Primitive{
 public:
  TriangleFanElement();
  TriangleFanElement(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in,  double alpha_in=1.0, int textured_in=0);
  ~TriangleFanElement();
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  bool isLine() const {return false;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
};

class TriangleFan: public TriangleFanElement {
 public:
  TriangleFan();
  ~TriangleFan(){};
  TriangleFan(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in,  double alpha_in=1.0, int textured_in=0);
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return false;};
};

class PolyCylinder : public Primitive{
  int accu;
  std::vector<Cartesian>colour_vector;
 public:
  PolyCylinder();
  PolyCylinder(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in, const std::vector<Cartesian> &colour_vector_in, double size_in=0.2, double alpha_in=1.0, int accu_in=4, int textured_in=0);
  ~PolyCylinder();
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  bool isLine() const {return false;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
  size_t GetNumberOfSimplePrimitives() const ;
};

class Ribbon : public Primitive{
 protected:
  int accu;
  int scale_steps;
  std::vector<Cartesian> n1_vertices;
  std::vector<Cartesian> n2_vertices;
  double thickness;
  double minsize;
  double maxsize;
  std::vector<Cartesian>colour_vector;
  virtual std::vector<Primitive*> GetSimplePrimitivesArrays(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) ;
  virtual void get_elliptical_ribbon_array(std::vector<Primitive*> &a,const std::vector<Cartesian> &vertices_tmp,const std::vector<std::vector<Cartesian> > &AB,int my_accu,int textured,int multicolour,const std::vector<Cartesian> &colour_vector_tmp, const double *colp, double alpha, const bool two_colour);
  virtual void get_flat_ribbon_array(std::vector<Primitive*> &a,const std::vector<Cartesian> &vertices_tmp,const std::vector<std::vector<Cartesian> > &AB,int my_accu,int textured,int multicolour,const std::vector<Cartesian> &colour_vector_tmp, const double *colp, double alpha, const bool two_colour);
  virtual void get_flat_rounded_ribbon_array(std::vector<Primitive*> &a,const std::vector<Cartesian> &vertices_tmp,const std::vector<std::vector<Cartesian> > &AB,int my_accu,int textured,int multicolour,const std::vector<Cartesian> &colour_vector_tmp, const double *colp, double alpha, const bool two_colour);
  virtual void get_fancy_ribbon_array(std::vector<Primitive*> &a,const std::vector<Cartesian> &vertices_tmp,const std::vector<std::vector<Cartesian> > &AB,int my_accu,int textured,int multicolour,const std::vector<Cartesian> &colour_vector_tmp, const double *colp, double alpha, const bool two_colour);
  int style;
  bool two_colour;
  bool edges_grey;
 public:
  Ribbon();
  Ribbon(const std::vector<Cartesian> &vertices_in, const std::vector<Cartesian> &n1_vertices_in, const std::vector<Cartesian> &n2_vertices_in, const double *colour_in, const Cartesian &origin_in, const std::vector<Cartesian> &colour_vector_in, double minsize_in=0.2, double maxsize_in=1.0, double thickness_in=0.2, double alpha_in=1.0, int accu_in=8, int scale_steps=8, int textured_in=0, int style_in=0, bool two_colour_in=false, bool edges_grey_in=false);
  ~Ribbon();
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  virtual std::vector<std::vector<Cartesian> > GetAB(int &insert_at_vertex) const;
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  bool isLine() const {return false;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
  virtual size_t GetNumberOfSimplePrimitives() const ;
  virtual void SetAlpha(double alpha_in);
};

class Worm : public Ribbon {
 public:
  Worm();
  Worm(const std::vector<Cartesian> &vertices_in, const std::vector<Cartesian> &n1_vertices_in, const std::vector<Cartesian> &n2_vertices_in, const double *colour_in, const Cartesian &origin_in, const std::vector<Cartesian> &colour_vector_in, double minsize_in=0.2, double maxsize_in=1.0, double thickness_in=0.2, double alpha_in=1.0, int accu_in=8, int scale_steps=8, int textured_in=0);
  ~Worm();
  void draw(const double *override_colour=0, int selective_override=0);
};

class VariableWorm : public Ribbon {
 public:
  VariableWorm();
  VariableWorm(const std::vector<Cartesian> &vertices_in, const std::vector<Cartesian> &n1_vertices_in, const std::vector<Cartesian> &n2_vertices_in, const double *colour_in, const Cartesian &origin_in, const std::vector<Cartesian> &colour_vector_in, double minsize_in=0.2, double maxsize_in=1.0, double thickness_in=0.2, double alpha_in=1.0, int accu_in=8, int scale_steps=8, int textured_in=0);
  ~VariableWorm();
  void draw(const double *override_colour=0, int selective_override=0);
  virtual std::vector<std::vector<Cartesian> > GetAB(int &insert_at_vertex) const;
};

class ArrowHeadRibbon : public Ribbon {
  double arrow_length;
  double arrow_width;
  std::vector<Cartesian> orig_vertices;
  std::vector<Cartesian> orig_n1_vertices;
  std::vector<Cartesian> orig_n2_vertices;
  std::vector<Cartesian> orig_colour_vector;
  int nstartarrow;
  virtual std::vector<Primitive*> GetSimplePrimitivesArrays(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) ;
 public:
  ArrowHeadRibbon();
  ArrowHeadRibbon(const std::vector<Cartesian> &vertices_in, const std::vector<Cartesian> &n1_vertices_in, const std::vector<Cartesian> &n2_vertices_in, const double *colour_in, const Cartesian &origin_in, const std::vector<Cartesian> &colour_vector_in, double minsize_in=0.2, double maxsize_in=1.0, double thickness_in=0.2, double arrow_length_in=2.2, double arrow_width_in=2.2, double alpha_in=1.0, int accu_in=8, int scale_steps=8, int textured_in=0, int style_in=0, bool two_colour_in=false, bool edges_grey_in=false);
  ~ArrowHeadRibbon();
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  std::vector<std::vector<Cartesian> > GetAB(int &insert_at_vertex) const ;
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  bool isLine() const {return false;};
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
  size_t GetNumberOfSimplePrimitives() const ;
};

enum { IMAGE_FULLSCREEN, IMAGE_ORIGINAL_SIZE, IMAGE_KEEP_ORIGINAL_SIZE };

class SimpleBillBoard {
  static GLdouble projMatrix[16];
  static GLdouble modelMatrix[16];
  static GLint viewport[4];
 public:
  virtual ~SimpleBillBoard(){};
  static double* GetProjectionMatrix();
  static double* GetModelviewMatrix();
  static GLint* GetViewport();
  static void SetMatrices();
  static int GetMagnification();
  virtual void draw(const double *override_colour=0, int selective_override=0) = 0 ;
  virtual void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v) = 0;
  virtual void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v)=0;
  bool isLine() const {return true;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
};

static bool force_load_textures;
class BillBoard : public Primitive, public SimpleBillBoard {
 protected:
   image_info image;
   unsigned int texture_id;
   unsigned int mask_texture_id;
   unsigned int texture_id_backup; // For when we are forced to get a new texture id (eg in different context).
   unsigned int mask_texture_id_backup; // For when we are forced to get a new texture id (eg in different context).
   int style;
   double first_scale_w;
   double first_scale_h;
   double scale_w;
   double scale_h;
   char *filename;
 public:
   BillBoard();
   BillBoard(const std::vector<Cartesian> &vertices_in, const Cartesian &origin_in, const char* filename_in, int style_in=IMAGE_ORIGINAL_SIZE);
  ~BillBoard();
  virtual void SetScale(double scale_w_in, double scale_h_in);
  void set_draw_colour(const GLfloat *col=0);
  virtual void draw(const double *override_colour=0, int selective_override=0);
  virtual void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  virtual void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  bool isLine() const {return false;};
  std::vector<double>  GetScale() const {std::vector<double> s; s.push_back(scale_w); s.push_back(scale_h); return s;};
  double  GetScaleW() const { return scale_w; }
  double  GetScaleH() const { return scale_h; }
  unsigned GetTextureID() const {return texture_id;};
  std::vector<int>  GetImageSize() const {std::vector<int> s; s.push_back(image.get_width()); s.push_back(image.get_height()); return s;};
  int GetImageWidth() const { return image.get_width(); }
  int GetImageHeight() const { return image.get_height(); }
  const char* GetFilename() const { return filename; }
  void SetTextureID(unsigned i) {texture_id=i;};
  void ClearTextureID() {texture_id=0;};
};

class SimpleText : public Primitive{
 protected:
  double baseline;
  std::string text;
  std::string family;
  std::string weight;
  std::string slant;
  int fn_size;
  int fn_descent;
  double yskip;
  int id;
  GLuint texture_id;
  GLuint texture_id_b;
  image_info texture;
  bool multicoloured;
  bool underline;
  bool strikeout;
  unsigned text_height;
  unsigned text_width;
  bool centered;
 public:
  void initialize(void);
  SimpleText();
  SimpleText(const Cartesian &vertex_in, const GLuint tex_id, const GLuint tex_id_b, const int width, const int height, const Cartesian &origin_in, const std::string &text_in, const int size_in=14, double alpha_in=1.0);
  SimpleText(const Cartesian &vertex_in, const std::string &text_in, const Cartesian &origin_in, double alpha_in=1.0, const std::string &family_in="times", const int size_in=14, const std::string &weight_in="", const std::string &slant_in="");
  void renderStringToPixmap();
  const bool &isMultiColoured() const { return multicoloured;} ;
  const image_info &GetTexture() const { return texture;} ;
  const GLuint &GetTextureID() const { return texture_id;} ;
  const GLuint &GetTextureID_B() const { return texture_id_b;} ;
  virtual ~SimpleText();
  virtual void SetRasterPosition(double x_in, double y_in, double z_in) const = 0;
  void set_draw_colour(const GLfloat *col=0);
  virtual void draw(const double *override_colour=0, int selective_override=0) = 0;
  virtual int IsBillBoard() const = 0;
  void draw_main(const double *override_colour=0, int selective_override=0);
  void SetFontName(const std::string &family_in, const int size_in, const std::string &weight_in, const std::string &slant_in);
  std::string StripTags();
  int LoadFont();
  //void BitMapFont(int count, int left, const MGFontInfo &finfo, int underline, int strikethrough);
  //void BackspaceBitMapFont(int count, int left, const MGFontInfo &finfo);
  //void BlankBitMapFont(int count, int left, const MGFontInfo &finfo);
  void SetFontSize(unsigned int fn_isize);
  void SetFontFamily(const std::string &Family);
  void SetFontWeight(const std::string &Weight);
  void SetFontSlant(const std::string &Slant);
  void SetText(const std::string &text_in);
  int GetFontSize() const;
  std::string GetFontFamily() const;
  std::string GetFontWeight() const;
  std::string GetFontSlant() const;
  std::string GetText(void) const;
  void SetDefaultColour(void);
  void SetColour(float r, float b, float g, float a);
  int GetID(void) const;
  bool isLine() const {return true;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
  void DrawPSmain(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v, bool is_a_bill_board);
  void SetFontDescent(unsigned int fn_isize);
  int GetFontDescent() const;
  void SetFontUnderline(bool underline_in){underline = underline_in;};
  void SetFontStrikeout(bool strikeout_in){strikeout = strikeout_in;};
  bool GetFontUnderline() const { return underline;};
  bool GetFontStrikeout() const { return strikeout;};
  unsigned GetTextWidth() const { return text_width;};
  unsigned GetTextHeight() const { return text_height;};
  void SetTextWidth(unsigned w_in) { text_width = w_in;};
  void SetTextHeight(unsigned h_in) { text_height = h_in;};
  void SetCentered(bool centered_in){centered = centered_in;};
  bool GetCentered() const { return centered;};
  void SetTextureID(unsigned i) {texture_id=i;};
  void ClearTextureID() {texture_id=0;};
  void SetTextureID_B(unsigned i) {texture_id_b=i;};
  void ClearTextureID_B() {texture_id_b=0;};
};

class Text : public SimpleText{
 public:
  Text();
  Text(const Cartesian &vertex_in, const GLuint tex_id, const GLuint tex_id_b, const int width, const int height, const Cartesian &origin_in, const std::string &text_in, const int size_in=14, double alpha_in=1.0);
  Text(const Cartesian &vertex_in, const std::string &text_in, const Cartesian &origin_in, double alpha_in=1.0, const std::string &family_in="times", const int size_in=14, const std::string &weight_in="", const std::string &slant_in="");
  ~Text();

  void draw(const double *override_colour=0, int selective_override=0);

  void SetRasterPosition(double x_in, double y_in, double z_in) const ;
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  int IsBillBoard() const {return 0;};
  bool isLine() const {return true;};
};

class BillBoardText : public SimpleText, public SimpleBillBoard {
 public:
  BillBoardText();
  BillBoardText(const Cartesian &vertex_in, const GLuint tex_id, const GLuint tex_id_b, const int width, const int height, const Cartesian &origin_in, const std::string &text_in, const int size_in=14, double alpha_in=1.0);
  BillBoardText(const Cartesian &vertex_in, const std::string &text_in, const Cartesian &origin_in, double alpha_in=1.0, const std::string &family_in="times", const int size_in=14, const std::string &weight_in="", const std::string &slant_in="");
  ~BillBoardText();

  void draw(const double *override_colour=0, int selective_override=0);

  void SetRasterPosition(double x_in, double y_in, double z_in) const ;
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  int IsBillBoard() const {return 1;};
  bool isLine() const {return true;};
};

class DashLineElement : public LineElement {
 private:
  double dash_length;
 public:
  DashLineElement();
  ~DashLineElement();
  DashLineElement(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return true;};
  void SetDashLength(double length) { dash_length = length; };
  double GetDashLength() const { return dash_length; };
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;

};

class PointCollection : public Primitive {
 protected:
  std::vector<Primitive*> lines;
 public:
  PointCollection();
  ~PointCollection();
  void add_primitive(Primitive *line);
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return true;};
  bool isDots() const {return true;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
  std::vector<Primitive*> GetPrimitives() const {return lines;}
  void SetPrimitives(std::vector<Primitive*> &lines_in){lines=lines_in;}
  size_t GetNumberOfSimplePrimitives() const { return lines.size();} ;
};

class LineCollection : public Primitive {
  std::vector<Primitive*> lines;
 public:
  LineCollection();
  ~LineCollection();
  void add_primitive(Primitive *line);
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  bool isLine() const {return true;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
  std::vector<Primitive*> GetPrimitives() const {return lines;}
  void SetPrimitives(std::vector<Primitive*> &lines_in){lines=lines_in;}
  size_t GetNumberOfSimplePrimitives() const { return lines.size();} ;
  std::vector<Primitive*> GetSimplePrimitivesArrays(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) ;
};

class Triangle: public TriangleElement{
 public:
  Triangle();
  ~Triangle(){};
  Triangle(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in,  double alpha_in=1.0, int textured_in=0);
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return false;};
};

class MGPolygon: public PolygonElement{
 public:
  MGPolygon();
  ~MGPolygon(){};
  MGPolygon(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in,  double alpha_in=1.0, int textured_in=0);
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return false;};
};

class Quad: public QuadElement{
 public:
  Quad();
  ~Quad(){};
  Quad(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in,  double alpha_in=1.0, int textured_in=0);
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return false;};
};

class Sphere: public SphereElement{
 public:
  Sphere();
  ~Sphere(){};
  Sphere(const Cartesian &vertex, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0, int accu_in=0, int textured_in=0);
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return false;};
};

class Spheroid: public SpheroidElement{
 public:
  Spheroid();
  ~Spheroid(){};
  Spheroid(const Cartesian &vertex, const double *colour_in, const Cartesian &origin_in, double a_in=1.0, double b_in=1.0, double c_in=1.0, const Cartesian& xaxis_in=Cartesian(1,0,0), const Cartesian& yaxis_in=Cartesian(0,1,0), const Cartesian& zaxis_in=Cartesian(0,0,1), double alpha_in=1.0, int accu_in=0, int show_axes=0, int show_solid=1, int textured_in=0);
  Spheroid(const Cartesian &vertex, const double *colour_in, const Cartesian &origin_in, const matrix &mat_in, double alpha_in=1.0, int accu_in=0, int show_axes_in=0, int show_solid_in=0, int textured_in=0);
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return false;};
};

class Cylinder: public CylinderElement{
 public:
  Cylinder();
  ~Cylinder(){};
  Cylinder(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0, int accu_in=8, int textured_in=0);
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return false;};
};

class DashCylinder: public DashCylinderElement{
 public:
  DashCylinder();
  ~DashCylinder(){};
  DashCylinder(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0, int accu_in=8, int textured_in=0);
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return false;};
};

class Cone: public ConeElement{
 public:
  Cone();
  ~Cone(){};
  Cone(const std::vector<Cartesian> &vertices_in, const double *colour_in, const Cartesian &origin_in, double size_in=1.0, double alpha_in=1.0, int accu_in=8, int textured_in=0);
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return false;};
};

class PolyCollection : public Primitive {
 protected:
  std::vector<Primitive*> prims;
 public:
   PolyCollection();
  ~PolyCollection();
  void add_primitive(Primitive *prims);
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  bool isLine() const {return false;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
  void SetAlpha(double alpha_in);
  void set_transparent(int trans_in);
  std::vector<Primitive*> GetPrimitives() const {return prims;}
  void SetPrimitives(std::vector<Primitive*> &prims_in){prims=prims_in;}
  size_t GetNumberOfSimplePrimitives() const { return prims.size();} ;
  void SetUseVBO(const int _useVBO); 
  void SetUseVertexArrays(const int _useVertexArrays); 
};

class SphereCollection : public PolyCollection {
 protected:
   int sphere_accu;
 public:
   SphereCollection();
  ~SphereCollection();
  virtual std::vector<Primitive*> GetSimplePrimitivesArrays(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) ;
   void SetAccu(int accu_in) {sphere_accu=accu_in;}; 
   int GetAccu() const { return sphere_accu;}; 
};

class ImposterSphereCollection : public Primitive {
  std::vector<Primitive*> prims;
 public:
   ImposterSphereCollection();
  ~ImposterSphereCollection();
  virtual std::vector<Primitive*> GetSimplePrimitivesArrays(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) ;
  void add_primitive(Primitive *prim);
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  bool isLine() const {return false;};
  bool isImposter() const {return true;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
  void SetAlpha(double alpha_in);
  void set_transparent(int trans_in);
  std::vector<Primitive*> GetPrimitives() const {return prims;}
  void SetPrimitives(std::vector<Primitive*> &prims_in){prims=prims_in;}
  size_t GetNumberOfSimplePrimitives() const { return prims.size();} ;
  void SetUseVBO(const int _useVBO); 
  void SetUseVertexArrays(const int _useVertexArrays); 
};

class SpheroidCollection : public SphereCollection {
 public:
   SpheroidCollection();
  ~SpheroidCollection();
  virtual std::vector<Primitive*> GetSimplePrimitivesArrays(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) ;
};

class TriangleCollection : public PolyCollection {
 public:
   TriangleCollection();
  ~TriangleCollection();
  virtual std::vector<Primitive*> GetSimplePrimitivesArrays(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) ;
};

class CylinderCollection : public PolyCollection {
 protected:
   int cylinder_accu;
 public:
   CylinderCollection();
  ~CylinderCollection();
  virtual std::vector<Primitive*> GetSimplePrimitivesArrays(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) ;
   void SetAccu(int accu_in) {cylinder_accu=accu_in;}; 
   int GetAccu() const { return cylinder_accu;}; 
};

class TorusElement : public Primitive{
   matrix orientation;
   double majorRadius;
   double minorRadius;
   double startAngle;
   double sweepAngle;
   double dash_length;
   bool isDashed;
 public:
  TorusElement();
  TorusElement(const Cartesian &vertex, const double *colour_in, const Cartesian &origin_in, double majorRadius_in=1.0, double minorRadius_in=0.1, double alpha_in=1.0, int textured_in=0);
  ~TorusElement();
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  bool isLine() const {return false;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
  void setMatrix(const matrix &a){orientation = a;} ;
  matrix getMatrix() const {return orientation;};
  void setStartAngle(double s){startAngle = s;} ;
  double getStartAngle() const {return startAngle;};
  void setSweepAngle(double s){sweepAngle = s;} ;
  double getSweepAngle() const {return sweepAngle;};
  void setMinorRadius(double s){minorRadius = s;} ;
  double getMinorRadius() const {return minorRadius;};
  void setMajorRadius(double s){majorRadius = s;} ;
  double getMajorRadius() const {return majorRadius;};
  double GetDashLength() const { return dash_length; };
  void SetDashLength(double length) { dash_length = length; };
  void setIsDashed(int dash) { isDashed = dash; };
  bool getIsDashed() const { return isDashed; };
};

class Torus : public TorusElement {
 public:
  Torus();
  Torus(const Cartesian &vertex, const double *colour_in, const Cartesian &origin_in, double majorRadius=1.0, double minorRadius=0.1, double alpha_in=1.0, int textured_in=0);
  ~Torus();
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return false;};
};

class ArcElement : public Primitive{
   matrix orientation;
   double startAngle;
   double sweepAngle;
   double width;
   double arrowLength;
   double arrowWidth;
   int arrow_head;
   double dash_length;
   bool isDashed;
 public:
  ArcElement();
  ArcElement(const Cartesian &vertex, const double *colour_in, const Cartesian &origin_in, double radius=1.0, double width_in=1.0, double alpha_in=1.0, int textured_in=0);
  ~ArcElement();
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  bool isLine() const {return false;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
  void setMatrix(const matrix &a){orientation = a;} ;
  matrix getMatrix() const {return orientation;};
  void setStartAngle(double s){startAngle = s;} ;
  double getStartAngle() const {return startAngle;};
  void setSweepAngle(double s){sweepAngle = s;} ;
  double getSweepAngle() const {return sweepAngle;};
  void setWidth(double s){width = s;} ;
  double getWidth() const {return width;};
  double GetDashLength() const { return dash_length; };
  void SetDashLength(double length) { dash_length = length; };
  int GetArrowHead() const { return arrow_head; };
  void SetArrowHead(int arrow_head_in) { arrow_head = arrow_head_in; };
  void setArrowLength(double length) { arrowLength = length; };
  double getArrowLength() const { return arrowLength; };
  void setArrowWidth(double Width) { arrowWidth = Width; };
  double getArrowWidth() const { return arrowWidth; };
  void setIsDashed(int dash) { isDashed = dash; };
  bool getIsDashed() const { return isDashed; };

};

class MGArc : public ArcElement {
 public:
  MGArc();
  MGArc(const Cartesian &vertex, const double *colour_in, const Cartesian &origin_in, double radius=1.0, double width_in=1.0, double alpha_in=1.0, int textured_in=0);
  ~MGArc();
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return true;};
};

class HelixElement : public Primitive{
   matrix orientation;
   double majorRadius;
   double minorRadius;
   double startAngle;
   double sweepAngle;
   double pitch;
   double dash_length;
   bool isDashed;
 public:
  HelixElement();
  HelixElement(const Cartesian &vertex, const double *colour_in, const Cartesian &origin_in, double majorRadius_in=1.0, double minorRadius_in=0.1, double pitch_in=1.0, double alpha_in=1.0, int textured_in=0);
  ~HelixElement();
  void set_draw_colour(const GLfloat *col=0);
  void draw(const double *override_colour=0, int selective_override=0);
  void DrawPovray(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, const Volume &v);
  void DrawPostScript(std::ofstream &fp, const Quat &quat, double radius, double ox, double oy, double oz, const matrix &objrotmatrix, const Cartesian &objorigin, double xoff, double yoff, double xscale, double yscale, double xscaleps, const Volume &v);
  bool isLine() const {return false;};
  std::vector<Primitive*> GetSimplePrimitives(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) const;
  void setMatrix(const matrix &a){orientation = a;} ;
  matrix getMatrix() const {return orientation;};
  void setStartAngle(double s){startAngle = s;} ;
  double getStartAngle() const {return startAngle;};
  void setSweepAngle(double s){sweepAngle = s;} ;
  double getSweepAngle() const {return sweepAngle;};
  void setMinorRadius(double s){minorRadius = s;} ;
  double getMinorRadius() const {return minorRadius;};
  void setMajorRadius(double s){majorRadius = s;} ;
  double getMajorRadius() const {return majorRadius;};
  void setPitch(double s){pitch = s;} ;
  double getPitch() const {return pitch;};
  double GetDashLength() const { return dash_length; };
  void SetDashLength(double length) { dash_length = length; };
  void setIsDashed(int dash) { isDashed = dash; };
  bool getIsDashed() const { return isDashed; };
};

class Helix : public HelixElement {
   double dash_length;
   bool isDashed;
 public:
  Helix();
  Helix(const Cartesian &vertex, const double *colour_in, const Cartesian &origin_in, double majorRadius=1.0, double minorRadius=0.1, double pitch_in=1.0, double alpha_in=1.0, int textured_in=0);
  ~Helix();
  void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return false;};
  double GetDashLength() const { return dash_length; };
  void SetDashLength(double length) { dash_length = length; };
  void setIsDashed(int dash) { isDashed = dash; };
  bool getIsDashed() const { return isDashed; };
};

class LinesCollection : public Primitive {
 protected:
  std::vector<double> line_cartesians;
  std::vector<double> line_colours;
  std::vector<int> line_colour_override;
 public:
  LinesCollection();
  ~LinesCollection();
  void add(const std::vector<Cartesian> &carts,const double *colour_array,int colour_override_in);
  virtual void draw(const double *override_colour=0, int selective_override=0);
  bool isLine() const {return true;};
  void set_draw_colour(const GLfloat *col=0){}
  virtual std::vector<Primitive*> GetSimplePrimitivesArrays(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) ;
  virtual void reserve(size_t nbonds);
  virtual std::vector<Primitive*> GetPrimitives() const;
};

class DashCylinderCollection : public CylinderCollection {
  double dash_length;
 public:
   DashCylinderCollection();
  ~DashCylinderCollection();
  virtual std::vector<Primitive*> GetSimplePrimitivesArrays(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) ;
  virtual std::vector<Primitive*> GetPrimitives() const;
  void SetDashLength(double length) { dash_length = length; };
  double GetDashLength() const { return dash_length; };
};

class DashLinesCollection : public LinesCollection {
  double dash_length;
 public:
  DashLinesCollection();
  ~DashLinesCollection();
  virtual void draw(const double *override_colour=0, int selective_override=0);
  virtual std::vector<Primitive*> GetSimplePrimitivesArrays(const Volume &clip_vol, const matrix &objrotmatrix, const Cartesian &objorigin, int start=-1, int end=-1) ;
  virtual void reserve(size_t nbonds);
  virtual std::vector<Primitive*> GetPrimitives() const;
  void SetDashLength(double length) { dash_length = length; };
  double GetDashLength() const { return dash_length; };
};

#endif