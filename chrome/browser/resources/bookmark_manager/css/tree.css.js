// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
  var a = 7;
  var a2 = a / 2;
  var ctx = document.getCSSCanvasContext('2d', 'triangle-filled', a2 + 2, a + 1);

  ctx.fillStyle = '#000';
  ctx.translate(.5, .5);
  ctx.beginPath();
  ctx.moveTo(0, 0);
  ctx.lineTo(0, a);
  ctx.lineTo(a2, a2);
  ctx.closePath();
  ctx.fill();
  ctx.stroke();

  var ctx = document.getCSSCanvasContext('2d', 'triangle-empty', a2 + 2, a + 1);

  ctx.strokeStyle = '#999';
  ctx.lineWidth  = 1.2;
  ctx.translate(.5, .5);
  ctx.fillStyle = '#000';
  ctx.beginPath();


  ctx.moveTo(0, 0);
  ctx.lineTo(0, a);
  ctx.lineTo(a2, a2);
  ctx.closePath();
  ctx.stroke();

  var ctx = document.getCSSCanvasContext('2d', 'triangle-hover', a2 + 2 + 4, a + 1 + 4);

  ctx.shadowColor = 'hsl(214,91%,89%)'
  ctx.shadowBlur = 3;
  ctx.shadowOffsetX = 1;
  ctx.shadowOffsetY = 0;

  ctx.strokeStyle = 'hsl(214,91%,79%)';
  ctx.lineWidth  = 1.2;
  ctx.translate(.5 + 2, .5 + 2);
  ctx.fillStyle = '#000';
  ctx.beginPath();

  ctx.moveTo(0, 0);
  ctx.lineTo(0, a);
  ctx.lineTo(a2, a2);
  ctx.closePath();
  ctx.stroke();
})();
