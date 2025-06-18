#!/usr/bin/env node
import { Pipeline } from "../dist/esm/index.mjs";

const fragment_shader = `
	#version 100
	#ifdef GL_ES
	precision mediump float;
	#endif
	varying vec2 v_texcoord;
	uniform sampler2D tex;
	void main () {
	  vec4 c = texture2D( tex, v_texcoord );
	  gl_FragColor = c.bgra;
	}
`;

let pipeline = new Pipeline("videotestsrc ! glupload ! glshader name=glshader ! glimagesink");

const glshader = pipeline.getElementByName("glshader");
glshader.setElementProperty("fragment", fragment_shader);

pipeline.play();

setInterval(() => console.log("running"), 1000);
