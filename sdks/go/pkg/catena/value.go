package catena

type StructVariantValue struct {
	StructVariantType string      `json:"struct_variant_type"`
	Value             any         `json:"value"`
}