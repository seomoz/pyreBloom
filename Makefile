build: pyreBloom/*.c
	python setup.py build_ext --inplace

install:
	python setup.py install

clean:
	# Remove the build
	sudo rm -rf build dist
	find . -name '*.pyc' | xargs -n 100 rm
	find . -name .coverage | xargs rm
	find . -name '*.o' | xargs rm
	find . -name '*.so' | xargs rm

.PHONY: test
test: install
	rm -f .coverage
	nosetests --exe -v
