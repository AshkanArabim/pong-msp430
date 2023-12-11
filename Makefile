all:
	(cd timerLib; make install)
	(cd lcdLib; make install)
	(cd buzzerLib; make install)
doc:
	rm -rf doxygen_docs
	doxygen Doxyfile
clean:
	(cd timerLib; make clean)
	(cd lcdLib; make clean)
	(cd buzzerLib; make clean)
	(cd pong; make clean)
	rm -rf lib h
	rm -rf doxygen_docs/*
