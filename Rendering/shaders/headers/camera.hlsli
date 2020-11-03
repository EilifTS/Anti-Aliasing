struct CameraBufferType
{
	matrix view_matrix;
	matrix inv_view_matrix;
	matrix projection_matrix;
	matrix inv_projection_matrix;
	matrix projection_matrix_no_jitter;
	matrix inv_projection_matrix_no_jitter;
};