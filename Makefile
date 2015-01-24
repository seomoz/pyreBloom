build: pyreBloom/*.c
	python setup.py build_ext --inplace

install:
	python setup.py install

clean:
	# Remove the build
	sudo rm -rf build dist
	find . -name '*.pyc' | xargs -n 100 rm -f
	find . -name .coverage | xargs rm -f
	find . -name '*.o' | xargs rm -f
	find . -name '*.so' | xargs rm -f

.PHONY: test
test:
	rm -f .coverage
	nosetests --exe -v
