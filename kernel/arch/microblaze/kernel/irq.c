

#include <linux/init.h>
#include <linux/ftrace.h>
#include <linux/kernel.h>
#include <linux/hardirq.h>
#include <linux/interrupt.h>
#include <linux/irqflags.h>
#include <linux/seq_file.h>
#include <linux/kernel_stat.h>
#include <linux/irq.h>

#include <asm/prom.h>

unsigned int irq_of_parse_and_map(struct device_node *dev, int index)
{
	struct of_irq oirq;

	if (of_irq_map_one(dev, index, &oirq))
		return NO_IRQ;

	return oirq.specifier[0];
}
EXPORT_SYMBOL_GPL(irq_of_parse_and_map);

static u32 concurrent_irq;

void __irq_entry do_IRQ(struct pt_regs *regs)
{
	unsigned int irq;
	struct pt_regs *old_regs = set_irq_regs(regs);

	irq_enter();
	irq = get_irq(regs);
next_irq:
	BUG_ON(irq == -1U);
	generic_handle_irq(irq);

	irq = get_irq(regs);
	if (irq != -1U) {
		pr_debug("next irq: %d\n", irq);
		++concurrent_irq;
		goto next_irq;
	}

	irq_exit();
	set_irq_regs(old_regs);
}

int show_interrupts(struct seq_file *p, void *v)
{
	int i = *(loff_t *) v, j;
	struct irqaction *action;
	unsigned long flags;

	if (i == 0) {
		seq_printf(p, "		");
		for_each_online_cpu(j)
			seq_printf(p, "CPU%-8d", j);
		seq_putc(p, '\n');
	}

	if (i < nr_irq) {
		raw_spin_lock_irqsave(&irq_desc[i].lock, flags);
		action = irq_desc[i].action;
		if (!action)
			goto skip;
		seq_printf(p, "%3d: ", i);
#ifndef CONFIG_SMP
		seq_printf(p, "%10u ", kstat_irqs(i));
#else
		for_each_online_cpu(j)
			seq_printf(p, "%10u ", kstat_cpu(j).irqs[i]);
#endif
		seq_printf(p, " %8s", irq_desc[i].status &
					IRQ_LEVEL ? "level" : "edge");
		seq_printf(p, " %8s", irq_desc[i].chip->name);
		seq_printf(p, "  %s", action->name);

		for (action = action->next; action; action = action->next)
			seq_printf(p, ", %s", action->name);

		seq_putc(p, '\n');
skip:
		raw_spin_unlock_irqrestore(&irq_desc[i].lock, flags);
	}
	return 0;
}

unsigned int irq_create_mapping(struct irq_host *host, irq_hw_number_t hwirq)
{
	return hwirq;
}
EXPORT_SYMBOL_GPL(irq_create_mapping);

unsigned int irq_create_of_mapping(struct device_node *controller,
					u32 *intspec, unsigned int intsize)
{
	return intspec[0];
}
EXPORT_SYMBOL_GPL(irq_create_of_mapping);
