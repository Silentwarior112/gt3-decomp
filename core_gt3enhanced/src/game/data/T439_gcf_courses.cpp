/* T439 - .gcf section type 0x8, the courses by list (? name),
 * 0x0021E210..0x0021E350. */
#include "game/data/gcf_internal.h"

/* Caller: gcf_relocate_course_list. */
// FUNC 0x0021E210 gcf_relocate_course
GcfCourse *gcf_relocate_course(GcfCourse *course)
{
    long off;

    course->name = GCF_REL(const char *, course, off, course->name);
    return course;
}

/* Caller: gcf_relocate_courses. */
// FUNC 0x0021E228 gcf_relocate_course_list
GcfCourseList *gcf_relocate_course_list(GcfCourseList *list)
{
    long off;
    int i = 0;

    list->name = GCF_REL(const char *, list, off, list->name);
    list->courses = GCF_REL(GcfCourse **, list, off, list->courses);
    for (; i < list->count; i++)
        list->courses[i] = gcf_relocate_course(GCF_REL(GcfCourse *, list, off, list->courses[i]));
    return list;
}

/* Caller: ProductConfig::ProductConfig. */
// FUNC 0x0021E2C8 gcf_relocate_courses
GcfCourseSection *gcf_relocate_courses(GcfCourseSection *sec)
{
    long off;
    int i = 0;

    sec->list = GCF_REL(GcfCourseList **, sec, off, sec->list);
    for (; i < sec->count; i++)
        sec->list[i] = gcf_relocate_course_list(GCF_REL(GcfCourseList *, sec, off, sec->list[i]));
    return sec;
}
