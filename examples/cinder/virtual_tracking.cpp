#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"
#include <list>
#include <random>

using namespace ci;
using namespace ci::app;

class BasicApp : public App {
public:
	glm::ivec2 mouse;
	std::vector<glm::vec3> points;
	std::map<size_t, std::list<glm::vec2>> trajectory;
	gl::VertBatchRef vert;
	gl::GlslProgRef mGlsl;
	CameraPersp	mCamera;
	CameraUi mCamUi;

	void setup() override {
		std::minstd_rand engine;
		std::uniform_real_distribution dist(-1.0, 1.0);
		auto rand = [&](){ return dist(engine); };
		size_t size = 256;
		points.reserve(size);
		for(size_t i = 0; i < size; i++) {
			points.emplace_back(rand(), rand(), rand());
		}

		vert = gl::VertBatch::create(GL_POINTS);
		for(const auto& p : points) { vert->color(1.0, 1.0, 1.0); vert->vertex(p); }

		mGlsl = gl::GlslProg::create(gl::GlslProg::Format()
			.vertex(R"(
				#version 150

				uniform mat4 ciModelViewProjection;

				in vec4 ciPosition;
				in vec4 ciColor;

				out VertexData {
					vec4 color;
				} vOut;

				void main() {
					vOut.color = ciColor;
					gl_Position	= ciModelViewProjection * ciPosition;
				}
			)")
			.geometry(R"(
				#version 150

				layout (points) in;
				layout (triangle_strip, max_vertices = 8) out;

				uniform ivec2 ciWindowSize;

				in VertexData {
					vec4 color;
				} vIn[];

				out VertexData {
					vec4 color;
				} vOut;

				vec2[] array = vec2[](
					vec2(-5.0, -0.5),
					vec2(-5.0, +0.5),
					vec2(+5.0, -0.5),
					vec2(+5.0, +0.5),
					vec2(-0.5, -5.0),
					vec2(+0.5, -5.0),
					vec2(-0.5, +5.0),
					vec2(+0.5, +5.0)
				);
				float scale = 2.0;

				void main() {
					for(int i = 0; i < array.length(); i++) {
						vOut.color = vIn[0].color;
						vec2 aspect = vec2(1.0 / float(ciWindowSize.x), 1.0 / float(ciWindowSize.y));
						vec3 pos = gl_in[0].gl_Position.xyz / gl_in[0].gl_Position.w;
						gl_Position = vec4(pos.xy + (array[i] * scale * aspect), pos.z, 1.0);
						EmitVertex();
						if (i % 4 == 3) { EndPrimitive(); }
					}
				}
			)")
			.fragment(R"(
				#version 150

				in VertexData {
					vec4 color;
				} vIn;
				out vec4 oColor;

				void main() {
					oColor = vIn.color;
				}
			)")
		);

		mCamera.lookAt(vec3(0, 0, 1), vec3(0, 0, 0));
		mCamUi = CameraUi(&mCamera);
	}

	void draw() override {
		mCamera.setAspectRatio(getWindowAspectRatio());
		mCamera.lookAt(mCamera.getEyePoint(), vec3(0.0));
		mat4 projectionMatrix = mCamera.getProjectionMatrix();
		mat4 viewMatrix = mCamera.getViewMatrix();
		mat4 pmMatrix = projectionMatrix * viewMatrix;

		for(size_t i = 0; i < points.size(); i++) {
			vec4 p = vec4(points[i], 1.0);
			p = pmMatrix * p;
			p /= p.w;
			if (
				p.x < -1.0 || 1.0 < p.x ||
				p.y < -1.0 || 1.0 < p.y ||
				p.z < -1.0 || 1.0 < p.z
			) {
				trajectory.erase(i);
				continue;
			}
			vec2 p2(p.x, -p.y);
			p2 = (p2 + vec2(1.0)) * vec2(getWindowSize() / 2);
			trajectory[i].emplace_back(p2);
			while(trajectory[i].size() > 128) trajectory[i].pop_front();
		}

		gl::clear(Color::gray(0.1f));
		{
			gl::ScopedGlslProg scpGlsl(mGlsl);
			gl::ScopedProjectionMatrix scpProjection(projectionMatrix);
			gl::ScopedModelMatrix scpModel(viewMatrix);
			vert->draw();
		}
		{
			for(const auto& t : trajectory) {
				auto vert2 = gl::VertBatch::create(GL_LINE_STRIP);
				size_t count = 0;
				for(auto itr = t.second.crbegin(); itr != t.second.crend(); itr++, count++) {
					vert2->color(1.0, 0.0, 0.0, std::max(0.0, 1.0 - static_cast<double>(count) / 12.0));
					vert2->vertex(*itr);
				}
				vert2->draw();
			}
		}
	}

	void mouseMove(MouseEvent event) override { mouse = event.getPos(); }
	void mouseUp(MouseEvent event) override { mCamUi.mouseUp(event); }
	void mouseDown(MouseEvent event) override { mCamUi.mouseDown(event); }
	void mouseWheel(MouseEvent event) override { mCamUi.mouseWheel(event); }
	void mouseDrag(MouseEvent event) override { mCamUi.mouseDrag(event); }
	void keyDown(KeyEvent e) override {
		switch(e.getCode()) {
		case KeyEvent::KEY_ESCAPE:
		case KeyEvent::KEY_q:
			quit();
			break;
		}
	}
};

CINDER_APP( BasicApp, RendererGl )
