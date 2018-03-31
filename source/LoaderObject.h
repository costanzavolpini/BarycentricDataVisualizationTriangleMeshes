#ifndef LOADER_H
#define LOADER_H

#include "Point3.h"
#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <map>
#include <iterator>

using namespace std;


/***************************************************************************
LoaderObject.h
Comment:  This file load the mesh using an .OFF file.
***************************************************************************/

// List of vertices and triangles
vector<Point3d> v;
struct Triangle { int v[3]; };
vector<Triangle> t;
double PI = atan(1)*4;
int number_triangles;

        bool load (const char * path, vector<float> &out_vertices, vector<float> &out_normals) {
            /**
                Read .OFF file
            */ 
            ifstream in(path); // substitute with path
            if (!in) {
                cout<<"\nError reading file."<<endl;
                return false;
            }
            string s;
            string offName = "OFF";
            getline(in,s);

            if (s.compare(0,offName.size(),offName)!=0) {
                cout << "This is not a valid OFF file." << endl;
                return false;
            }

            int i, dummy, num_triangles, num_vertices;
            in >> num_vertices >> num_triangles >> dummy;

            v.resize(num_vertices);
            for (i = 0; i < num_vertices; i++)
                in >> v[i][0] >> v[i][1] >> v[i][2];

            t.resize(num_triangles);

            for (i = 0; i < num_triangles; i++)
                in >> dummy >> t[i].v[0] >> t[i].v[1] >> t[i].v[2];

            in.close();
            number_triangles = num_triangles;

            // vertices array
            vector<Point3d> normals(num_vertices);
            std::fill(normals.begin(), normals.end(), Point3d(0.0f, 0.0f, 0.0f));

            vector<float> triangle_vertices(num_triangles * 9);
            vector<float> triangle_normals(num_triangles * 9);

            // save normals
            vector<int> v_counter(num_vertices);
            std::fill(v_counter.begin(), v_counter.end(), 0); // initialize every vertex normal to (0,0,0)

            // save everything with the color into vertices --- 
            for (int k = 0; k < num_triangles; k++) {    

                Point3d v1 = v[t[k].v[0]];
                Point3d v2 = v[t[k].v[1]];
                Point3d v3 = v[t[k].v[2]];

                // for every triangle face compute face normal and normalize it
                Point3d n = (v2-v1)^(v3-v1);
                n.normalize();

                normals[t[k].v[0]] += n;
                v_counter[t[k].v[0]]++; // update counter

                normals[t[k].v[1]] += n;
                v_counter[t[k].v[1]]++; // update counter

                normals[t[k].v[2]] += n;
                v_counter[t[k].v[2]]++; // update counter
            }


            // normalize every vertex normal
            // average of norms of adj triangle of a vertex (sum of triangle norms / number of triangles)
            for(int k = 0; k < num_vertices; k++){
                // normals[k].normalize();
                if(v_counter[k] != 0){ 
                    normals[k] = normals[k] / v_counter[k]; 
               }
                normals[k].normalize();
            }

            for (int k = 0; k < num_triangles; k++) {
                // insert vertice values in triangles
                triangle_vertices[9 * k] = v[t[k].v[0]].x();
                triangle_vertices[9 * k + 1] = v[t[k].v[0]].y();
                triangle_vertices[9 * k + 2] = v[t[k].v[0]].z();
                
                triangle_vertices[9 * k + 3] = v[t[k].v[1]].x();
                triangle_vertices[9 * k + 4] = v[t[k].v[1]].y();
                triangle_vertices[9 * k + 5] = v[t[k].v[1]].z();

                triangle_vertices[9 * k + 6] = v[t[k].v[2]].x();
                triangle_vertices[9 * k + 7] = v[t[k].v[2]].y();
                triangle_vertices[9 * k + 8] = v[t[k].v[2]].z();

                // insert normal values in triangles
                triangle_normals[9 * k] = normals[t[k].v[0]].x();
                triangle_normals[9 * k + 1] = normals[t[k].v[0]].y();
                triangle_normals[9 * k + 2] = normals[t[k].v[0]].z();
                
                triangle_normals[9 * k + 3] = normals[t[k].v[1]].x();
                triangle_normals[9 * k + 4] = normals[t[k].v[1]].y();
                triangle_normals[9 * k + 5] = normals[t[k].v[1]].z();

                triangle_normals[9 * k + 6] = normals[t[k].v[2]].x();
                triangle_normals[9 * k + 7] = normals[t[k].v[2]].y();
                triangle_normals[9 * k + 8] = normals[t[k].v[2]].z();
            }



            // -------------- GAUSSIAN CURVATURE ----------------- REIMPLEMENT
            // find gaussian curvature
            vector<float> triangle_gc(num_triangles * 9);

            // iterate inside triangles
            for(int i = 0; i < num_triangles; i++){
                Point3d v0 = Point3d(triangle_vertices[9 * i], triangle_vertices[9 * i + 1], triangle_vertices[9 * i + 2]);
                Point3d v1 = Point3d(triangle_vertices[9 * i + 3], triangle_vertices[9 * i + 4], triangle_vertices[9 * i + 5]);
                Point3d v2 = Point3d(triangle_vertices[9 * i + 6], triangle_vertices[9 * i + 7], triangle_vertices[9 * i + 8]);

                // calculate gc for each vertex of triangle
                // VERTEX 1
                // v1 -> v0 -> v2
                Point3d v1v0 = v0 - v1;
                Point3d v0v2 = v2 - v0;
                double sum_angles_1 = v1v0.getAngle(v0v2);

                // VERTEX 2
                // v2 -> v1 -> v0
                Point3d v2v1 = v1 - v2;
                double sum_angles_2 = v2v1.getAngle(v1v0);


                // VERTEX 3
                // v0 -> v2 -> v1
                double sum_angles_3 = v0v2.getAngle(v2v1);

                // // for each triangle-vertex selected, search all vertices
                // for(int k = 0; k < num_vertices; k++){
                //     if(v[k] ==  v0){
                        
                //     }
                    
                // }
                // double gaussian_curvature_j = 2 * PI - sum_angles;
            }
           // -------------- END GAUSSIAN CURVATURE -----------------


            // output vectors
            //For each vertex of each triangle
            out_vertices.clear();
            out_normals.clear();

            for (unsigned int i = 0; i < triangle_vertices.size(); i++) {
                // get value
                out_vertices.push_back(triangle_vertices[i]);
                out_normals.push_back(triangle_normals[i]);           
            }

            cout << "Object loaded" << endl;
            return true;
        }


int get_number_triangles(){
    return number_triangles;
}


#endif
