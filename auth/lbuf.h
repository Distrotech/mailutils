struct _line_buffer;

int _auth_lb_create (struct _line_buffer **s);
void _auth_lb_destroy (struct _line_buffer **s);
void _auth_lb_drop (struct _line_buffer *s);

int _auth_lb_grow (struct _line_buffer *s, const char *ptr, size_t size);
int _auth_lb_read (struct _line_buffer *s, const char *ptr, size_t size);
int _auth_lb_readline (struct _line_buffer *s, const char *ptr, size_t size);
int _auth_lb_level (struct _line_buffer *s);
char *_auth_lb_data (struct _line_buffer *s);


