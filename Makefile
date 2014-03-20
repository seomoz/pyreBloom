build: pyreBloom/*.c
	python setup.py build_ext --inplace

install:
	python setup.py install

clean:
	# Remove the build
	sudo rm -rf build dist
	# And all of our pyc files
	find . -name '*.pyc' | xargs -n 100 rm
	# And lastly, .coverage files
	find . -name .coverage | xargs rm

.PHONY: test
test: install
	rm -f .coverage
	nosetests --exe -v
