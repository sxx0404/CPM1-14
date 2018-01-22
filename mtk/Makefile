all:
	$(MAKE) -C libs all
	$(MAKE) -C mac all
	$(MAKE) -C ht all
	$(MAKE) -C Logd all
	$(MAKE) -C bn all
	cp -rf files/* down/
clean:
	rm -rf down
	$(MAKE) -C mac clean
	$(MAKE) -C libs clean
	$(MAKE) -C ht clean
	$(MAKE) -C Logd clean
	$(MAKE) -C bn clean
rebuilt:clean all
