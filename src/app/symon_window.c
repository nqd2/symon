#include "symon_window.h"

#include "linux_cpu.h"
#include "linux_memory.h"
#include "scheduler.h"

#include <glib.h>

typedef struct {
    gatomicrefcount ref_count;
    GtkLabel *cpu_value;
    GtkLabel *memory_value;
    GtkLabel *swap_value;
    GtkLabel *load_value;
    GtkLabel *status_label;
    GtkDropDown *refresh_dropdown;
    SymonLinuxCpuCollector cpu_collector;
    SymonLinuxMemoryCollector memory_collector;
    SymonScheduler scheduler;
    gboolean scheduler_initialized;
    gboolean closed;
} SymonWindowState;

typedef struct {
    SymonWindowState *state;
    SymonSnapshot snapshot;
} PendingSnapshot;

static const guint32 refresh_intervals_ms[] = {250, 500, 1000, 2000, 5000};

static SymonWindowState *window_state_ref(SymonWindowState *state)
{
    g_atomic_ref_count_inc(&state->ref_count);
    return state;
}

static void window_state_unref(SymonWindowState *state)
{
    if (g_atomic_ref_count_dec(&state->ref_count)) {
        g_free(state);
    }
}

static char *format_percent(double value)
{
    return g_strdup_printf("%.1f%%", value);
}

static char *format_gibibytes(uint64_t bytes)
{
    const double gibibytes = (double)bytes / (1024.0 * 1024.0 * 1024.0);

    return g_strdup_printf("%.1f GiB", gibibytes);
}

static void set_label_text(GtkLabel *label, char *value)
{
    gtk_label_set_text(label, value);
    g_free(value);
}

static gboolean apply_snapshot_on_main_context(gpointer user_data)
{
    PendingSnapshot *pending = user_data;
    SymonWindowState *state = pending->state;
    const SymonSnapshot *snapshot = &pending->snapshot;

    if (!state->closed) {
        if (snapshot->cpu.usage_available) {
            set_label_text(state->cpu_value, format_percent(snapshot->cpu.total_usage_percent));
        } else {
            gtk_label_set_text(state->cpu_value, "Sampling...");
        }

        if (snapshot->memory.available) {
            set_label_text(state->memory_value, format_gibibytes(snapshot->memory.used_bytes));
            set_label_text(state->swap_value, format_gibibytes(snapshot->memory.swap_used_bytes));
        }
        if (snapshot->cpu.available) {
            char *load =
                g_strdup_printf("%.2f  %.2f  %.2f", snapshot->cpu.load_average_1m,
                                snapshot->cpu.load_average_5m, snapshot->cpu.load_average_15m);
            set_label_text(state->load_value, load);
        }

        if (snapshot->collectors_failed == 0) {
            gtk_label_set_text(state->status_label, "Monitoring live system data");
        } else {
            gtk_label_set_text(state->status_label, "Some metrics are unavailable");
        }
    }

    window_state_unref(state);
    g_free(pending);
    return G_SOURCE_REMOVE;
}

static void queue_snapshot(const SymonSnapshot *snapshot, void *user_data)
{
    SymonWindowState *state = user_data;
    PendingSnapshot *pending = g_try_new(PendingSnapshot, 1);

    if (pending == NULL) {
        return;
    }

    pending->state = window_state_ref(state);
    pending->snapshot = *snapshot;
    g_main_context_invoke(NULL, apply_snapshot_on_main_context, pending);
}

static gboolean start_scheduler(SymonWindowState *state, guint32 interval_ms)
{
    SymonSchedulerConfig config = {
        .interval_ms = interval_ms,
        .on_snapshot = queue_snapshot,
        .user_data = state,
    };
    SymonError error;

    if (state->scheduler_initialized) {
        symon_scheduler_destroy(&state->scheduler);
        state->scheduler_initialized = FALSE;
    }

    symon_error_clear(&error);
    if (!symon_scheduler_init(&state->scheduler, &config, &error)) {
        gtk_label_set_text(state->status_label, "Cannot initialize monitoring");
        return FALSE;
    }
    state->scheduler_initialized = TRUE;

    if (!symon_scheduler_add_collector(
            &state->scheduler, symon_linux_cpu_as_collector(&state->cpu_collector), &error) ||
        !symon_scheduler_add_collector(
            &state->scheduler, symon_linux_memory_as_collector(&state->memory_collector), &error) ||
        !symon_scheduler_start(&state->scheduler, &error)) {
        gtk_label_set_text(state->status_label, "Cannot start monitoring");
        symon_scheduler_destroy(&state->scheduler);
        state->scheduler_initialized = FALSE;
        return FALSE;
    }

    gtk_label_set_text(state->status_label, "Starting collection...");
    return TRUE;
}

static void refresh_changed(GtkDropDown *dropdown, GParamSpec *parameter, gpointer user_data)
{
    SymonWindowState *state = user_data;
    guint selection = gtk_drop_down_get_selected(dropdown);

    (void)parameter;
    if (selection >= G_N_ELEMENTS(refresh_intervals_ms)) {
        return;
    }

    (void)start_scheduler(state, refresh_intervals_ms[selection]);
}

static void destroy_window_state(gpointer user_data)
{
    SymonWindowState *state = user_data;

    state->closed = TRUE;
    if (state->scheduler_initialized) {
        symon_scheduler_destroy(&state->scheduler);
        state->scheduler_initialized = FALSE;
    }
    window_state_unref(state);
}

GtkWindow *symon_window_new(GtkApplication *application)
{
    g_autoptr(GtkBuilder) builder =
        gtk_builder_new_from_resource("/io/github/symon/SyMon/ui/main-window.ui");
    GtkWindow *window = GTK_WINDOW(gtk_builder_get_object(builder, "main_window"));
    SymonWindowState *state = g_try_new0(SymonWindowState, 1);

    gtk_window_set_application(window, application);
    if (state == NULL) {
        GtkLabel *status_label = GTK_LABEL(gtk_builder_get_object(builder, "status_label"));

        gtk_label_set_text(status_label, "Monitoring unavailable");
        return window;
    }

    g_atomic_ref_count_init(&state->ref_count);
    state->cpu_value = GTK_LABEL(gtk_builder_get_object(builder, "cpu_value"));
    state->memory_value = GTK_LABEL(gtk_builder_get_object(builder, "memory_value"));
    state->swap_value = GTK_LABEL(gtk_builder_get_object(builder, "swap_value"));
    state->load_value = GTK_LABEL(gtk_builder_get_object(builder, "load_value"));
    state->status_label = GTK_LABEL(gtk_builder_get_object(builder, "status_label"));
    state->refresh_dropdown = GTK_DROP_DOWN(gtk_builder_get_object(builder, "refresh_dropdown"));
    symon_linux_cpu_collector_init(&state->cpu_collector, "/proc/stat", "/proc/loadavg");
    symon_linux_memory_collector_init(&state->memory_collector, "/proc/meminfo");

    g_object_set_data_full(G_OBJECT(window), "symon-window-state", state, destroy_window_state);
    g_signal_connect(state->refresh_dropdown, "notify::selected", G_CALLBACK(refresh_changed),
                     state);
    (void)start_scheduler(state, refresh_intervals_ms[1]);

    return window;
}
