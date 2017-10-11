CC ?= gcc

EXCLUDE = examples/CROMU_00063/ \
	examples/CROMU_00070/ \
	examples/CROMU_00073/ \
	examples/CROMU_00079/ \
	examples/CROMU_00083/ \
	examples/CROMU_00093/ \
	examples/CROMU_00095/ \
	examples/EAGLE_00004/ \
	examples/KPRCA_00075/ \
	examples/KPRCA_00081/ \
	examples/KPRCA_00091/ \
	examples/KPRCA_00093/ \
	examples/KPRCA_00097/ \
	examples/KPRCA_00111/ \
	examples/KPRCA_00119/ \
	examples/LUNGE_00005/ \
	examples/NRFIN_00066/ \
	examples/NRFIN_00067/ \
	examples/NRFIN_00072/ \
	examples/TNETS_00002/ \
	examples/YAN01_00015/ \
	cqe-challenges/CROMU_00006/ \
	cqe-challenges/CROMU_00028/ \
	cqe-challenges/KPRCA_00016/ \
	cqe-challenges/KPRCA_00024/ \
	cqe-challenges/KPRCA_00025/ \
	cqe-challenges/KPRCA_00026/ \
	cqe-challenges/KPRCA_00044/ \
	cqe-challenges/KPRCA_00048/ \
	cqe-challenges/KPRCA_00051/ \
	cqe-challenges/NRFIN_00006/ \
	cqe-challenges/YAN01_00009/

ALL_DIRS = $(dir $(wildcard examples/*/. cqe-challenges/*/.))
CHALLENGE_DIRS = $(filter-out $(EXCLUDE),$(ALL_DIRS))
CHALLENGES = $(foreach d,$(CHALLENGE_DIRS),$(d)$(lastword $(subst /, ,$(d))))
POLLERS = $(foreach d,$(CHALLENGE_DIRS),$(d)pollers)
TEST_LOGS = $(foreach d,$(CHALLENGE_DIRS),$(d)test.log)
OBJS = $(foreach d,$(CHALLENGE_DIRS),$(d)obj)

all: $(CHALLENGES) $(POLLERS)
challenges: $(CHALLENGES)
check: $(TEST_LOGS)

$(CHALLENGES):
	@echo $@
	@cd $(dir $@) && build.sh $(notdir $@) $(CC) $(CFLAGS)

%/pollers:
	@cd $*/poller/for-release && generate-polls machine.py state-graph.yaml .

%/test.log:
	-@cb-test --directory $* --cb $(lastword $(subst /, ,$*)) --xml_dir $*/poller/for-release --log $@ --debug
	@grep "polls " $@

clean:
	@rm -vf $(CHALLENGES)
	@rm -vf $(TEST_LOGS)
	@rm -rvf $(OBJS)
