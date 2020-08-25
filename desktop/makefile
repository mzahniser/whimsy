CCX = g++
CFLAGS = -Wall -O3 --std=c++17
LIBS = -lpng -lSDL2_image -lSDL2


.PHONY : all
all : whimsy editor masks svg export glyphs


whimsy: whimsy.o Avatar.o Data.o Dialog.o Edge.o Font.o Interaction.o Menu.o Paths.o Point.o Polygon.o Rect.o Ring.o Room.o Sprite.o Text.o Variables.o World.o
	$(CCX) -o $@ $^ $(LIBS)

whimsy.o: whimsy.cpp Avatar.h Color.h Data.h Dialog.h Edge.h Font.h Interaction.h Menu.h Paths.h Point.h Polygon.h Rect.h Ring.h Room.h Sprite.h Text.h Variables.h World.h
	$(CCX) -c $(CFLAGS) -o $@ $<


editor: editor.o Canvas.o Data.o Edge.o Font.o Interaction.o Palette.o Point.o Polygon.o Rect.o Ring.o Room.o Sprite.o
	$(CCX) -o $@ $^ $(LIBS)

editor.o: editor.cpp Canvas.h Color.h Data.h Edge.h Font.h Interaction.h Palette.h Point.h Polygon.h Rect.h Ring.h Room.h Sprite.h
	$(CCX) -c $(CFLAGS) -o $@ $<


masks: masks.o Canvas.o Data.o Edge.o Point.o Polygon.o Rect.o Ring.o
	$(CCX) -o $@ $^ $(LIBS)

masks.o: masks.cpp Canvas.h Color.h Data.h Edge.h Point.h Polygon.h Rect.h Ring.h
	$(CCX) -c $(CFLAGS) -o $@ $<


svg: svg.o
	$(CCX) -o $@ $^ $(LIBS)

svg.o: svg.cpp
	$(CCX) -c $(CFLAGS) -o $@ $<


export: export.o
	$(CCX) -o $@ $^ $(LIBS)

export.o: export.cpp
	$(CCX) -c $(CFLAGS) -o $@ $<


glyphs: glyphs.o
	$(CXX) -o $@ $^ `pkg-config --libs freetype2`

glyphs.o: glyphs.cpp
	$(CXX) -c $(CFLAGS) -o $@ $< `pkg-config --cflags freetype2`


Avatar.o: Avatar.cpp Avatar.h Data.h Point.h Room.h
	$(CCX) -c $(CFLAGS) -o $@ $<

Canvas.o: Canvas.cpp Canvas.h Color.h Point.h Polygon.h Rect.h Ring.h
	$(CCX) -c $(CFLAGS) -o $@ $<

Data.o: Data.cpp Data.h Point.h
	$(CCX) -c $(CFLAGS) -o $@ $<

Dialog.o: Dialog.cpp Color.h Data.h Dialog.h Point.h Rect.h Sprite.h Text.h Variables.h World.h
	$(CCX) -c $(CFLAGS) -o $@ $<

Edge.o: Edge.cpp Edge.h Point.h Ring.h
	$(CCX) -c $(CFLAGS) -o $@ $<

Font.o: Font.cpp Color.h Data.h Font.h Point.h Rect.h
	$(CCX) -c $(CFLAGS) -o $@ $<

Interaction.o: Interaction.cpp Data.h Interaction.h Point.h
	$(CCX) -c $(CFLAGS) -o $@ $<

Menu.o: Menu.cpp Color.h Data.h Font.h Menu.h Sprite.h
	$(CCX) -c $(CFLAGS) -o $@ $<

Palette.o: Palette.cpp Color.h Data.h Font.h Palette.h Point.h Rect.h Sprite.h
	$(CCX) -c $(CFLAGS) -o $@ $<

Paths.o: Paths.cpp Paths.h Point.h Polygon.h Room.h Sprite.h
	$(CCX) -c $(CFLAGS) -o $@ $<

Point.o: Point.cpp Point.h
	$(CCX) -c $(CFLAGS) -o $@ $<

Polygon.o: Polygon.cpp Edge.h Point.h Polygon.h Ring.h
	$(CCX) -c $(CFLAGS) -o $@ $<

Rect.o: Rect.cpp Point.h Rect.h
	$(CCX) -c $(CFLAGS) -o $@ $<

Ring.o: Ring.cpp Edge.h Point.h Ring.h
	$(CCX) -c $(CFLAGS) -o $@ $<

Room.o: Room.cpp Color.h Data.h Interaction.h Point.h Polygon.h Rect.h Room.h Sprite.h
	$(CCX) -c $(CFLAGS) -o $@ $<

Sprite.o: Sprite.cpp Data.h Point.h Polygon.h Rect.h Sprite.h
	$(CCX) -c $(CFLAGS) -o $@ $<

Text.o: Text.cpp Font.h Point.h Text.h
	$(CCX) -c $(CFLAGS) -o $@ $<

Variables.o: Variables.cpp Variables.h
	$(CCX) -c $(CFLAGS) -o $@ $<

World.o: World.cpp Avatar.h Color.h Data.h Dialog.h Font.h Interaction.h Menu.h Paths.h Point.h Room.h Sprite.h Variables.h World.h
	$(CCX) -c $(CFLAGS) -o $@ $<


.PHONY: clean
clean:
	rm -f whimsy editor masks svg export glyphs *.o
